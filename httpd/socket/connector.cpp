#include "socket/connector.h"
shared_ptr<Connection> Connector::connect(string &ip, uint16_t port,
                                          bool is_non_blocking) {
    shared_ptr<Socket> socket =
        make_shared<Socket>(Socket::domain_INET, is_non_blocking);
    socket->connect(ip, port);
    return make_shared<Connection>(socket, is_non_blocking);
}