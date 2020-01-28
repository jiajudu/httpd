#include "server/preThreadedServer.h"
#include "auxiliary/error.h"
#include <algorithm>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
PreThreadedServer::PreThreadedServer(string &_ip, uint16_t _port,
                                     int _numThreads)
    : ip(_ip), port(_port), numThreads(_numThreads), tasks(_numThreads) {
}
void PreThreadedServer::run() {
    if (numThreads <= 0) {
        exit(1);
    }
    vector<thread> threads;
    for (int i = 0; i < numThreads; i++) {
        threads.push_back(thread(&PreThreadedServer::workerMain, this));
    }
    listenSocket = make_shared<Socket>(Socket::domainINET, false, false);
    listenSocket->bind(ip, port);
    listenSocket->listen(10);
    while (true) {
        shared_ptr<Socket> conn = listenSocket->accept();
        tasks.push(conn);
    }
}
void PreThreadedServer::workerMain() {
    while (true) {
        shared_ptr<Socket> conn = tasks.pop();
        char buf[4096];
        ssize_t size = conn->recv(buf, 4096);
        while (size > 0) {
            onMessage(buf, size, bind(&Socket::_send, conn, _1, _2));
            size = conn->recv(buf, 4096);
        }
        conn->close();
    }
}
