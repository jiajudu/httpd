#include "server/processPoolReactorServer.h"
#include "auxiliary/error.h"
#include <algorithm>
#include <iostream>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
ProcessPoolReactorServer::ProcessPoolReactorServer(string &_ip, uint16_t _port,
                                                   int _numProcess)
    : Server(_ip, _port), numProcess(_numProcess) {
}
void ProcessPoolReactorServer::run() {
    if (numProcess <= 0) {
        exit(1);
    }
    for (int i = 0; i < numProcess; i++) {
        int fds[2];
        int ret = socketpair(Socket::domain_local, SOCK_STREAM, 0, fds);
        if (ret < 0) {
            syscall_error();
        }
        int pid = fork();
        if (pid < 0) {
            syscall_error();
        }
        if (pid > 0) {
            close(fds[1]);
            child_fds.push_back(fds[0]);
        } else {
            for (int j = 0; j < i; j++) {
                close(child_fds[j]);
            }
            close(fds[0]);
            child_main(fds[1]);
        }
    }
    listener = make_shared<Listener>(ip, port, 10, true);
    for (long i = 0;; i++) {
        shared_ptr<Connection> conn = listener->accept();
        send_conn(child_fds[i % numProcess], conn);
        conn->close();
    }
}
void ProcessPoolReactorServer::child_main(int fd) {
    unordered_map<int, shared_ptr<Connection>> sockets;
    while (true) {
        vector<struct pollfd> pollfds;
        pollfds.push_back({fd, POLLIN, 0});
        for (auto it = sockets.begin(); it != sockets.end(); it++) {
            short int events = POLLIN | POLLPRI | POLLRDHUP;
            if (it->second->has_content_to_send()) {
                events |= POLLOUT;
            }
            pollfds.push_back({it->first, events, 0});
        }
        int ret = poll(&pollfds[0], pollfds.size(), -1);
        if (ret < 0) {
            syscall_error();
        }
        for (auto &pollfd : pollfds) {
            if (pollfd.fd == fd) {
                if (pollfd.revents & POLLIN) {
                    shared_ptr<Connection> conn = recv_conn(fd);
                    sockets[conn->get_fd()] = conn;
                }
            } else {
                shared_ptr<Connection> conn = sockets[pollfd.fd];
                if (pollfd.revents & POLLNVAL) {
                    agreement_error("poll invalid fd");
                } else if (pollfd.revents & POLLERR) {
                    sockets.erase(pollfd.fd);
                    conn->close();
                } else {
                    if (pollfd.revents & POLLIN) {
                        conn->non_blocking_recv();
                        string message;
                        conn->recv(message, decoder);
                        if (message.size() > 0) {
                            onMessage(message, [&](string &s) -> size_t {
                                return conn->send(s);
                            });
                        }
                    }
                    if (pollfd.revents & POLLOUT) {
                        conn->non_blocking_send();
                    }
                    if (pollfd.revents & POLLRDHUP) {
                        sockets.erase(pollfd.fd);
                        conn->close();
                    }
                }
            }
        }
    }
}
void ProcessPoolReactorServer::send_conn(int fd, shared_ptr<Connection> conn) {
    if (!conn->can_be_sent()) {
        agreement_error("this connection cannot be sent");
    }
    struct iovec iov[1];
    struct msghdr msg;
    char buf[16];
    int connfd = conn->get_socket()->get_fd();
    int domain = conn->get_socket()->get_domain();
    bool non_blocking = conn->get_is_non_blocking();
    memcpy(buf, &domain, sizeof(int));
    memcpy(buf + 8, &non_blocking, sizeof(bool));
    iov[0].iov_base = buf;
    iov[0].iov_len = 16;
    msg.msg_name = 0;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    cmsghdr cm;
    cm.cmsg_len = CMSG_LEN(sizeof(int));
    cm.cmsg_level = SOL_SOCKET;
    cm.cmsg_type = SCM_RIGHTS;
    *(int *)CMSG_DATA(&cm) = connfd;
    msg.msg_control = &cm;
    msg.msg_controllen = CMSG_LEN(sizeof(int));
    ssize_t ret = sendmsg(fd, &msg, 0);
    if (ret < 0) {
        syscall_error();
    }
}
shared_ptr<Connection> ProcessPoolReactorServer::recv_conn(int fd) {
    struct iovec iov[1];
    struct msghdr msg;
    char buf[24];
    iov[0].iov_base = buf;
    iov[0].iov_len = 16;
    msg.msg_name = 0;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    struct cmsghdr cm;
    msg.msg_control = &cm;
    msg.msg_controllen = CMSG_LEN(sizeof(int));
    recvmsg(fd, &msg, 0);
    int connfd = *(int *)CMSG_DATA(&cm);
    int domain = *(reinterpret_cast<int *>(buf));
    bool non_blocking = *(reinterpret_cast<bool *>(buf + 8));
    return make_shared<Connection>(
        make_shared<Socket>(connfd, domain, non_blocking), non_blocking);
}