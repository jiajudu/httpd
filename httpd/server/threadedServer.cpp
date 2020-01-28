#include "server/threadedServer.h"
#include "auxiliary/error.h"
#include <thread>
#include <unistd.h>
using std::bind;
using std::make_shared;
using std::thread;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
ThreadedServer::ThreadedServer(string &ip, uint16_t port) {
    listenSocket = make_shared<Socket>(Socket::domainINET, false, false);
    listenSocket->bind(ip, port);
    listenSocket->listen(10);
}
void ThreadedServer::run() {
    while (true) {
        shared_ptr<Socket> conn = listenSocket->accept();
        printf("aaa");
        thread t(&ThreadedServer::threadRun, this, conn);
        t.detach();
    }
}
void ThreadedServer::threadRun(shared_ptr<Socket> conn) {
    char buf[4096];
    ssize_t size = conn->recv(buf, 4096);
    while (size > 0) {
        onMessage(buf, size, bind(&Socket::_send, conn, _1, _2));
        size = conn->recv(buf, 4096);
    }
    conn->close();
}