#include "server/iterativeServer.h"
#include <sys/signal.h>
IterativeServer::IterativeServer(shared_ptr<Service> _service, string &_ip,
                                 uint16_t _port)
    : Server(_service, _ip, _port) {
}
void IterativeServer::run() {
    listener = make_shared<Listener>(ip, port, 10);
    signal(SIGPIPE, SIG_IGN);
    while (true) {
        shared_ptr<Connection> conn = listener->accept();
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
}
