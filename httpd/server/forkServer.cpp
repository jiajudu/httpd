#include "server/forkServer.h"
#include "auxiliary/error.h"
#include <unistd.h>
using std::bind;
using std::make_shared;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
ForkServer::ForkServer(string &ip, uint16_t port) {
    listenSocket = make_shared<Socket>(Socket::domainINET, false, false);
    listenSocket->bind(ip, port);
    listenSocket->listen(10);
}
void ForkServer::run() {
    while (true) {
        shared_ptr<Socket> conn = listenSocket->accept();
        int pid = fork();
        if (pid < 0) {
            fatalError();
        }
        if (pid > 0) {
            conn->close();
        } else {
            listenSocket->close();
            char buf[4096];
            ssize_t size = conn->recv(buf, 4096);
            while (size > 0) {
                onMessage(buf, size, bind(&Socket::_send, conn, _1, _2));
                size = conn->recv(buf, 4096);
            }
            conn->close();
            exit(0);
        }
    }
}