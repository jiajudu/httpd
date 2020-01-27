#include "server/iterativeServer.h"
using std::make_shared;
IterativeServer::IterativeServer(string &ip, uint16_t port) {
    listenSocket = make_shared<Socket>(Socket::domainINET, false, false);
    listenSocket->bind(ip, port);
    listenSocket->listen(10);
}
void IterativeServer::run() {
    while (true) {
        shared_ptr<Socket> conn = listenSocket->accept();
        char buf[4096];
        ssize_t size = conn->recv(buf, 4096);
        while (size > 0) {
            conn->send(buf, size);
            size = conn->recv(buf, 4096);
        }
    }
}
ssize_t IterativeServer::send(shared_ptr<Socket> conn, char *buf,
                              uint64_t size) {
    return conn->send(buf, size);
}