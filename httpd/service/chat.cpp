#include "service/chat.h"
void Chat::onConnection(shared_ptr<Connection> conn) {
    conns.insert(conn);
}
void Chat::onMessage(shared_ptr<Connection> conn, string &input_message) {
    for (auto c : conns) {
        if (c != conn) {
            c->send(input_message);
        }
    }
}
void Chat::onDisconnect(shared_ptr<Connection> conn) {
    conns.erase(conn);
}