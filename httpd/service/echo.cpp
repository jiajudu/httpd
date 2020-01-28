#include "service/echo.h"
Echo::Echo(shared_ptr<Server> server_) : server(server_) {
    server->onMessage = bind(&Echo::onMessage, this, _1, _2, _3);
}
void Echo::onMessage(char *buf, ssize_t size,
                     function<ssize_t(char *buf, ssize_t size)> send) {
    send(buf, size);
}