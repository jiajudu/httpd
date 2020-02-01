#include "service/echo.h"
Echo::Echo() {
    decoder = bind(&Echo::decode, this, _1, _2);
}
void Echo::onMessage(shared_ptr<Connection> conn, string &input_message) {
    conn->send(input_message);
}
size_t Echo::decode(char *s, size_t n) {
    (void)s;
    return n;
}
