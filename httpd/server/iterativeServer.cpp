#include "server/iterativeServer.h"
IterativeServer::IterativeServer(string &_ip, uint16_t _port)
    : ip(_ip), port(_port) {
}
void IterativeServer::run() {
    listenSocket = make_shared<Socket>(Socket::domainINET, false, false);
    listenSocket->bind(ip, port);
    listenSocket->listen(10);
    while (true) {
        shared_ptr<Socket> conn = listenSocket->accept();
        char buf[4096];
        ssize_t size = conn->recv(buf, 4096);
        while (size > 0) {
            onMessage(buf, size, bind(&Socket::_send, conn, _1, _2));
            size = conn->recv(buf, 4096);
        }
        conn->close();
    }
}
