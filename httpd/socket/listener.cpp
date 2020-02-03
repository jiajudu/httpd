#include "socket/listener.h"
Listener::Listener(string &ip, uint16_t port, int backlog) {
    socket = make_shared<Socket>(Socket::domain_INET);
    socket->bind(ip, port);
    socket->listen(backlog);
}
shared_ptr<Connection> Listener::accept() {
    int fd = socket->accept();
    return make_shared<Connection>(
        make_shared<Socket>(fd, socket->get_domain()));
}
int Listener::close() {
    if (onClose) {
        onClose();
    }
    return socket->close();
}
int Listener::get_fd() const {
    return socket->get_fd();
}