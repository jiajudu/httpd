#include "service/limitedEcho.h"
void LimitedEcho::onMessage(shared_ptr<Connection> conn,
                            string &input_message) {
    conn->send(input_message);
}
void LimitedEcho::onConnection(shared_ptr<Connection> conn) {
    conn->set_deactivation(10);
}