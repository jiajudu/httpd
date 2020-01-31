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
    vector<FDTransmission> child_fds;
    for (int i = 0; i < numProcess; i++) {
        FDTransmission fdt;
        int pid = fork();
        if (pid < 0) {
            syscall_error();
        }
        if (pid > 0) {
            fdt.parent();
            child_fds.push_back(fdt);
        } else {
            for (int j = 0; j < i; j++) {
                child_fds[j].child();
            }
            fdt.child();
            child_main(fdt);
        }
    }
    listener = make_shared<Listener>(ip, port, 10, true);
    for (long i = 0;; i++) {
        shared_ptr<Connection> conn = listener->accept();
        child_fds[i % numProcess].send_conn(conn);
        conn->close();
    }
}
void ProcessPoolReactorServer::child_main(FDTransmission &fdt) {
    unordered_map<int, shared_ptr<Connection>> sockets;
    while (true) {
        vector<struct pollfd> pollfds;
        pollfds.push_back({fdt.get_fd(), POLLIN, 0});
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
            if (pollfd.fd == fdt.get_fd()) {
                if (pollfd.revents & POLLIN) {
                    shared_ptr<Connection> conn = fdt.recv_conn();
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
