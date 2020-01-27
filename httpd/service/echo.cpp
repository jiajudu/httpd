#include "service/echo.h"
using std::bind;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
Echo::Echo(shared_ptr<Server> server_) : server(server_) {
    server->onMessage = bind(&Echo::onMessage, this, _1, _2, _3);
}
void Echo::onMessage(char *buf, uint64_t size,
                     function<ssize_t(char *buf, uint64_t size)> send) {
    send(buf, size);
}