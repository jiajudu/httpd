#include "server/reactorServer.h"
#include "auxiliary/error.h"
#include <poll.h>
#include <unordered_set>
ReactorServer::ReactorServer(string &_ip, uint16_t _port)
    : ip(_ip), port(_port) {
}
void ReactorServer::run() {
    listenSocket = make_shared<Socket>(Socket::domainINET, false, false);
    listenSocket->bind(ip, port);
    listenSocket->listen(10);
    unordered_map<int, shared_ptr<Socket>> sockets;
    while (true) {
        vector<struct pollfd> pollfds(sockets.size() + 1);
        pollfds[0].fd = listenSocket->getFd();
        pollfds[0].events = POLLIN;
        pollfds[0].revents = 0;
        int i = 1;
        for (auto it = sockets.begin(); it != sockets.end(); it++) {
            pollfds[i].fd = it->first;
            pollfds[i].events = POLLIN;
            pollfds[i].revents = 0;
            i++;
        }
        int ret = poll(&pollfds[0], pollfds.size(), -1);
        if (ret < 0) {
            fatalError();
        }
        for (auto &pollfd : pollfds) {
            if (pollfd.revents & POLLIN) {
                if (pollfd.fd == listenSocket->getFd()) {
                    shared_ptr<Socket> conn = listenSocket->accept();
                    sockets[conn->getFd()] = conn;
                } else if (sockets.find(pollfd.fd) != sockets.end()) {
                    vector<char> buf(4096);
                    shared_ptr<Socket> conn = sockets[pollfd.fd];
                    ssize_t size = conn->recv(buf, buf.size());
                    if (size > 0) {
                        onMessage(buf, size, bind(&Socket::send, conn, _1, _2));
                    } else {
                        sockets.erase(pollfd.fd);
                        conn->close();
                    }
                }
            }
        }
    }
}
