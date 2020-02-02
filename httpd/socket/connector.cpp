#include "socket/connector.h"
shared_ptr<Connection> Connector::connect(string &ip, uint16_t port) {
    shared_ptr<Socket> socket = make_shared<Socket>(Socket::domain_INET);
    socket->connect(ip, port);
    return make_shared<Connection>(socket);
}