#include "server/preThreadedServer.h"
#include "auxiliary/error.h"
#include <algorithm>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
PreThreadedServer::PreThreadedServer(string &_ip, uint16_t _port,
                                     int _numThreads)
    : Server(_ip, _port), numThreads(_numThreads), tasks(_numThreads) {
}
void PreThreadedServer::run() {
    if (numThreads <= 0) {
        exit(1);
    }
    vector<thread> threads;
    for (int i = 0; i < numThreads; i++) {
        threads.push_back(thread(&PreThreadedServer::workerMain, this));
    }
    listener = make_shared<Listener>(ip, port, 10);
    while (true) {
        shared_ptr<Connection> conn = listener->accept();
        tasks.push(conn);
    }
}
void PreThreadedServer::workerMain() {
    while (true) {
        shared_ptr<Connection> conn = tasks.pop();
        string buf(4096, 0);
        size_t size = conn->recv(buf);
        while (size > 0) {
            string message(buf.begin(), buf.begin() + size);
            onMessage(message,
                      [&](string &s) -> size_t { return conn->send(s); });
            size = conn->recv(buf);
        }
        conn->close();
    }
}
