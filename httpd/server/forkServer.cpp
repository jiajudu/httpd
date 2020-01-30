#include "server/forkServer.h"
#include "auxiliary/error.h"
#include <unistd.h>
ForkServer::ForkServer(string &_ip, uint16_t _port) : Server(_ip, _port) {
}
void ForkServer::run() {
    listener = make_shared<Listener>(ip, port, 10);
    while (true) {
        shared_ptr<Connection> conn = listener->accept();
        int pid = fork();
        if (pid < 0) {
            syscall_error();
        }
        if (pid > 0) {
            conn->close();
        } else {
            listener->close();
            string buf(4096, 0);
            size_t size = conn->recv(buf);
            while (size > 0) {
                string message(buf.begin(), buf.begin() + size);
                onMessage(message,
                          [&](string &s) -> size_t { return conn->send(s); });
                size = conn->recv(buf);
            }
            conn->close();
            exit(0);
        }
    }
}
