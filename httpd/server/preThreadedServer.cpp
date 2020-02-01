#include "server/preThreadedServer.h"
#include "auxiliary/error.h"
#include <algorithm>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
PreThreadedServer::PreThreadedServer(shared_ptr<Service> _service, string &_ip,
                                     uint16_t _port, int _numThreads)
    : Server(_service, _ip, _port), numThreads(_numThreads),
      tasks(_numThreads) {
}
void PreThreadedServer::run() {
    if (numThreads <= 0) {
        exit(1);
    }
    vector<thread> threads;
    for (int i = 0; i < numThreads; i++) {
        threads.push_back(thread(&PreThreadedServer::worker_main, this));
    }
    listener = make_shared<Listener>(ip, port, 10);
    while (true) {
        shared_ptr<Connection> conn = listener->accept();
        tasks.push(conn);
    }
}
void PreThreadedServer::worker_main() {
    while (true) {
        shared_ptr<Connection> conn = tasks.pop();
        service->onConnection(conn);
        string buf(4096, 0);
        size_t size = 4096;
        while (conn->active()) {
            if (size > 0) {
                size = conn->recv(buf);
                string message(buf.begin(), buf.begin() + size);
                service->onMessage(conn, message);
            } else {
                conn->close();
            }
        }
        conn->close();
    }
}
