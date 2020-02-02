#include "server/threadedServer.h"
#include "auxiliary/error.h"
#include <sys/signal.h>
#include <thread>
#include <unistd.h>
ThreadedServer::ThreadedServer(shared_ptr<Service> _service, string &_ip,
                               uint16_t _port, ServerOption &server_option)
    : Server(_service, _ip, _port, server_option) {
}
void ThreadedServer::run() {
    listener = make_shared<Listener>(ip, port, 10);
    signal(SIGPIPE, SIG_IGN);
    while (true) {
        shared_ptr<Connection> conn = listener->accept();
        bool allow = increase_connection_counter();
        if (allow) {
            thread t(&ThreadedServer::threadRun, this, conn);
            t.detach();
        } else {
            conn->close();
        }
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
    decrease_connection_counter();
}
bool ThreadedServer::increase_connection_counter() {
    lock_guard<mutex> g(mutex);
    if (active_connection_number < option.max_connection_number) {
        active_connection_number++;
        return true;
    } else {
        return false;
    }
}
bool ThreadedServer::decrease_connection_counter() {
    lock_guard<mutex> g(mutex);
    active_connection_number--;
    return true;
}