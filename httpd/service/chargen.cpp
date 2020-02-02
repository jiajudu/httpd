#include "service/chargen.h"
Chargen::Chargen() {
    for (int i = 0; i < 128; i++) {
        chars.push_back(static_cast<char>(i));
    }
}
void Chargen::onConnection(shared_ptr<Connection> conn) {
    if (conn->get_is_non_blocking()) {
        conn->send(chars);
    } else {
        while (conn->active()) {
            conn->send(chars);
        }
    }
}
void Chargen::onSendComplete(shared_ptr<Connection> conn) {
    conn->send(chars);
}
