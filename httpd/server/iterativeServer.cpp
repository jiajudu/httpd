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
        vector<char> buf(4096);
        ssize_t size = conn->recv(buf, buf.size());
        while (size > 0) {
            onMessage(buf, size, bind(&Socket::send, conn, _1, _2));
            size = conn->recv(buf, buf.size());
        }
        conn->close();
    }
}
