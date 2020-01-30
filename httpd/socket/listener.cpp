#include "socket/listener.h"
Listener::Listener(string &ip, uint16_t port, int backlog,
                   bool is_non_blocking_)
    : is_non_blocking(is_non_blocking_) {
    socket = make_shared<Socket>(Socket::domain_INET, is_non_blocking);
    socket->bind(ip, port);
    socket->listen(backlog);
}
shared_ptr<Connection> Listener::accept() {
    int fd = socket->accept();
    return make_shared<Connection>(
        make_shared<Socket>(fd, socket->get_domain(), is_non_blocking),
        is_non_blocking);
}
int Listener::close() {
    return socket->close();
}
int Listener::get_fd() const {
    return socket->get_fd();
}