#include "server/threadedServer.h"
#include "auxiliary/error.h"
#include <thread>
#include <unistd.h>
ThreadedServer::ThreadedServer(string &_ip, uint16_t _port)
    : Server(_ip, _port) {
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
    string buf(4096, 0);
    size_t size = conn->recv(buf);
    while (size > 0) {
        string message(buf.begin(), buf.begin() + size);
        onMessage(message, [&](string &s) -> size_t { return conn->send(s); });
        size = conn->recv(buf);
    }
    conn->close();
}