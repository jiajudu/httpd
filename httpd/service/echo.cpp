#include "service/echo.h"
Echo::Echo(shared_ptr<Server> server_) : server(server_) {
    server->onMessage = bind(&Echo::onMessage, this, _1, _2);
    server->decoder = bind(&Echo::decode, this, _1, _2);
}
void Echo::onMessage(string &input_message,
                     function<size_t(string &output_message)> send) {
    send(input_message);
}
size_t Echo::decode(char *s, size_t n) {
    (void)s;
    return n;
}
