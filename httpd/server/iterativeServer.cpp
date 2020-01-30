#include "server/iterativeServer.h"
IterativeServer::IterativeServer(string &_ip, uint16_t _port)
    : Server(_ip, _port) {
}
void IterativeServer::run() {
    listener = make_shared<Listener>(ip, port, 10);
    while (true) {
        shared_ptr<Connection> conn = listener->accept();
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
