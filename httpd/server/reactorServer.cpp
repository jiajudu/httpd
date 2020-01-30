#include "server/reactorServer.h"
#include "auxiliary/error.h"
#include <poll.h>
#include <unordered_set>
ReactorServer::ReactorServer(string &_ip, uint16_t _port) : Server(_ip, _port) {
}
void ReactorServer::run() {
    listener = make_shared<Listener>(ip, port, 10, true);
    unordered_map<int, shared_ptr<Connection>> sockets;
    while (true) {
        vector<struct pollfd> pollfds(sockets.size() + 1);
        pollfds[0].fd = listener->get_fd();
        pollfds[0].events = POLLIN;
        pollfds[0].revents = 0;
        int i = 1;
        for (auto it = sockets.begin(); it != sockets.end(); it++) {
            pollfds[i].fd = it->first;
            pollfds[i].events = POLLIN | POLLPRI | POLLRDHUP;
            if (it->second->has_content_to_send()) {
                pollfds[i].events |= POLLOUT;
            }
            pollfds[i].revents = 0;
            i++;
        }
        int ret = poll(&pollfds[0], pollfds.size(), -1);
        if (ret < 0) {
            syscall_error();
        }
        for (auto &pollfd : pollfds) {
            if (pollfd.fd == listener->get_fd()) {
                if (pollfd.revents & POLLIN) {
                    shared_ptr<Connection> conn = listener->accept();
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
