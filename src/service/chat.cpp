#include "service/chat.h"
void Chat::onConnection(shared_ptr<Connection> conn) {
    // TO DO: not compatible with blocking io
    lock_guard<mutex> g(lock);
    conns.insert(conn);
}
void Chat::onMessage(shared_ptr<Connection> conn, string &input_message) {
    lock_guard<mutex> g(lock);
    for (auto c : conns) {
        if (c != conn) {
            c->send(input_message);
        }
    }
}
void Chat::onDisconnect(shared_ptr<Connection> conn) {
    lock_guard<mutex> g(lock);
    conns.erase(conn);
}