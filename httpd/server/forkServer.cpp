#include "server/forkServer.h"
#include "auxiliary/error.h"
#include <sys/signal.h>
#include <unistd.h>
ForkServer::ForkServer(shared_ptr<Service> _service, string &_ip,
                       uint16_t _port)
    : Server(_service, _ip, _port) {
}
void ForkServer::run() {
    listener = make_shared<Listener>(ip, port, 10);
    signal(SIGPIPE, SIG_IGN);
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
            exit(0);
        }
    }
}
