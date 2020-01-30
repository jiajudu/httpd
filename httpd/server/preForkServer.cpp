#include "server/preForkServer.h"
#include "auxiliary/error.h"
#include <algorithm>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
ChildProcess::ChildProcess(int _fd) : fd(_fd), busy(false) {
}
PreForkServer::PreForkServer(string &_ip, uint16_t _port, int _numProcess)
    : Server(_ip, _port), numProcess(_numProcess) {
}
void PreForkServer::run() {
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
            childs.push_back(ChildProcess(fds[0]));
        } else {
            for (int j = 0; j < i; j++) {
                close(childs[j].fd);
            }
            close(fds[0]);
            childMain(fds[1]);
        }
    }
    listener = make_shared<Listener>(ip, port, 10);
    while (true) {
        shared_ptr<Connection> conn = listener->accept();
        size_t childIndex = getAvailableProcess();
        sendConn(childs[childIndex].fd, conn);
        childs[childIndex].busy = true;
        conn->close();
    }
}
size_t PreForkServer::getAvailableProcess() {
    for (size_t i = 0; i < childs.size(); i++) {
        if (!childs[i].busy) {
            return i;
        }
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    int nfds = 0;
    for (size_t i = 0; i < childs.size(); i++) {
        FD_SET(childs[i].fd, &readfds);
        nfds = max(nfds, childs[i].fd);
    }
    int ret = select(nfds + 1, &readfds, 0, 0, 0);
    if (ret < 0) {
        syscall_error();
    }
    for (size_t i = 0; i < childs.size(); i++) {
        if (FD_ISSET(childs[i].fd, &readfds)) {
            char buf[1];
            read(childs[i].fd, buf, 1);
            childs[i].busy = false;
        }
    }
    for (size_t i = 0; i < childs.size(); i++) {
        if (!childs[i].busy) {
            return i;
        }
    }
    exit(1);
}
void PreForkServer::childMain(int fd) {
    while (true) {
        shared_ptr<Connection> conn = recvConn(fd);
        conn->close();
        string buf(4096, 0);
        size_t size = conn->recv(buf);
        while (size > 0) {
            string message(buf.begin(), buf.begin() + size);
            onMessage(message,
                      [&](string &s) -> size_t { return conn->send(s); });
            size = conn->recv(buf);
        }
        conn->close();
        write(fd, &buf[0], 1);
    }
}
void PreForkServer::sendConn(int fd, shared_ptr<Connection> conn) {
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
shared_ptr<Connection> PreForkServer::recvConn(int fd) {
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