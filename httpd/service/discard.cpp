#include "service/discard.h"
#include <functional>
Discard::Discard() {
    decoder = bind(&Discard::decode, this, _1, _2);
}
void Discard::onMessage(shared_ptr<Connection> conn, string &input_message) {
    (void)conn;
    (void)input_message;
}
size_t Discard::decode(char *s, size_t n) {
    (void)s;
    return n;
}
