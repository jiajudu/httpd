#include "socket/listener.h"
#include "schedule/listenerPool.h"
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
    if (pool) {
        pool->remove_listener(shared_from_this());
    }
    return socket->close();
}
int Listener::get_fd() const {
    return socket->get_fd();
}