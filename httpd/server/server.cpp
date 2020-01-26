#include "server/server.h"
Server::Server(std::string &ip, uint16_t port) {
    listenSocket = std::make_shared<Socket>(Socket::domainINET, false, false);
    listenSocket->bind(ip, port);
    listenSocket->listen(10);
}
void Server::run() {
    while (true) {
        std::shared_ptr<Socket> conn = listenSocket->accept();
        char buf[4096];
        ssize_t size = conn->recv(buf, 4096);
        while (size > 0) {
            conn->send(buf, size);
            size = conn->recv(buf, 4096);
        }
    }
}