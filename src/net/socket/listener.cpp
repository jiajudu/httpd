#include "net/socket/listener.h"
#include "net/schedule/listenerPool.h"
Listener::Listener(string &ip, uint16_t _port, int backlog) : port(_port) {
    socket = make_shared<Socket>(Socket::domain_INET);
    socket->reuse_addr();
    socket->bind(ip, port);
    socket->listen(backlog);
}
shared_ptr<Connection> Listener::accept() {
    string remote_ip;
    uint16_t remote_port;
    int fd = socket->accept(remote_ip, remote_port);
    shared_ptr<Socket> s = make_shared<Socket>(fd, socket->get_domain());
    string local_ip;
    uint16_t local_port;
    s->get_name(local_ip, local_port);
    return make_shared<Connection>(s, local_ip, local_port, remote_ip,
                                   remote_port);
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