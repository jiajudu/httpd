#include "schedule/connectionPool.h"
#include "auxiliary/error.h"
#include "schedule/poller.h"
#include <poll.h>
ConnectionPool::ConnectionPool(shared_ptr<Multiplexer> _multiplexer,
                               shared_ptr<Service> _service)
    : multiplexer(_multiplexer), service(_service) {
    eh = make_shared<EventHandler>();
    eh->read = bind(&ConnectionPool::read_callback, this, _1);
    eh->write = bind(&ConnectionPool::write_callback, this, _1);
    eh->error = bind(&ConnectionPool::error_callback, this, _1);
    eh->hang_up = bind(&ConnectionPool::hang_up_callback, this, _1);
}
void ConnectionPool::add_connection(shared_ptr<Connection> connection) {
    conns[connection->get_fd()] = connection;
    multiplexer->add_fd(connection->get_fd(), true, false, eh);
    connection->onClose = [this](shared_ptr<Connection> _c) -> void {
        conns.erase(_c->get_fd());
        multiplexer->del_fd(_c->get_fd());
        service->onDisconnect(_c);
    };
    connection->onSendBegin = [this](shared_ptr<Connection> _c) -> void {
        multiplexer->mod_fd(_c->get_fd(), true, true);
    };
    connection->onSendComplete = [this](shared_ptr<Connection> _c) -> void {
        multiplexer->mod_fd(_c->get_fd(), true, false);
        service->onSendComplete(_c);
    };
    service->onConnection(connection);
}
size_t ConnectionPool::size() {
    return conns.size();
}
void ConnectionPool::read_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd];
    conn->non_blocking_recv();
    string message;
    conn->recv(message);
    if (message.size() > 0) {
        service->onMessage(conn, message);
    }
}
void ConnectionPool::write_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd];
    conn->non_blocking_send();
}
void ConnectionPool::error_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd];
    multiplexer->del_fd(conn->get_fd());
    service->onDisconnect(conn);
    conn->shutdown();
}
void ConnectionPool::hang_up_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd];
    conn->close();
}
