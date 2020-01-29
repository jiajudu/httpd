#include "server/threadedServer.h"
#include "auxiliary/error.h"
#include <thread>
#include <unistd.h>
ThreadedServer::ThreadedServer(string &_ip, uint16_t _port)
    : ip(_ip), port(_port) {
}
void ThreadedServer::run() {
    listenSocket = make_shared<Socket>(Socket::domainINET, false, false);
    listenSocket->bind(ip, port);
    listenSocket->listen(10);
    while (true) {
        shared_ptr<Socket> conn = listenSocket->accept();
        thread t(&ThreadedServer::threadRun, this, conn);
        t.detach();
    }
}
void ThreadedServer::threadRun(shared_ptr<Socket> conn) {
    vector<char> buf(4096);
    ssize_t size = conn->recv(buf, buf.size());
    while (size > 0) {
        onMessage(buf, size, bind(&Socket::send, conn, _1, _2));
        size = conn->recv(buf, buf.size());
    }
    conn->close();
}