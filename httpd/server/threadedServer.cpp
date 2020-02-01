#include "server/threadedServer.h"
#include "auxiliary/error.h"
#include <thread>
#include <unistd.h>
ThreadedServer::ThreadedServer(shared_ptr<Service> _service, string &_ip,
                               uint16_t _port)
    : Server(_service, _ip, _port) {
}
void ThreadedServer::run() {
    listener = make_shared<Listener>(ip, port, 10);
    while (true) {
        shared_ptr<Connection> conn = listener->accept();
        thread t(&ThreadedServer::threadRun, this, conn);
        t.detach();
    }
}
void ThreadedServer::threadRun(shared_ptr<Connection> conn) {
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
}