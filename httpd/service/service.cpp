#include "service/service.h"
void Service::onConnection(shared_ptr<Connection> conn) {
    (void)conn;
}
void Service::onMessage(shared_ptr<Connection> conn, string &input_message) {
    (void)conn;
    (void)input_message;
}
void Service::onSendComplete(shared_ptr<Connection> conn) {
    (void)conn;
}