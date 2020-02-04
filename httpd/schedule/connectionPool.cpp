#include "schedule/connectionPool.h"
#include "auxiliary/error.h"
#include "schedule/poller.h"
#include <poll.h>
ConnectionPool::ConnectionPool(shared_ptr<Multiplexer> _multiplexer)
    : multiplexer(_multiplexer) {
    if (multiplexer->connection_pool != 0) {
        fatal_error("Duplicated connection pool.");
    } else {
        multiplexer->connection_pool = shared_from_this();
    }
    eh = make_shared<EventHandler>();
    eh->read = bind(&ConnectionPool::read_callback, this, _1);
    eh->write = bind(&ConnectionPool::write_callback, this, _1);
    eh->error = bind(&ConnectionPool::error_callback, this, _1);
    eh->hang_up = bind(&ConnectionPool::hang_up_callback, this, _1);
}
void ConnectionPool::add_connection(shared_ptr<Connection> connection,
                                    shared_ptr<ConnectionEvent> event) {
    conns[connection->get_fd()] = make_pair(connection, event);
    multiplexer->add_fd(connection->get_fd(), true, false, eh);
    if (event) {
        event->onConnection(connection);
    }
}
size_t ConnectionPool::size() {
    return conns.size();
}
void ConnectionPool::read_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd].first;
    shared_ptr<ConnectionEvent> event = conns[fd].second;
    conn->non_blocking_recv();
    string message;
    conn->recv(message);
    if (message.size() > 0 && event->onMessage) {
        event->onMessage(conn, message);
    }
}
void ConnectionPool::write_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd].first;
    conn->non_blocking_send();
}
void ConnectionPool::error_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd].first;
    shared_ptr<ConnectionEvent> event = conns[fd].second;
    multiplexer->del_fd(conn->get_fd());
    if (event->onDisconnect) {
        event->onDisconnect(conn);
    }
    conn->shutdown();
}
void ConnectionPool::hang_up_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd].first;
    conn->close();
}
void ConnectionPool::onClose(shared_ptr<Connection> _c) {
    conns.erase(_c->get_fd());
    multiplexer->del_fd(_c->get_fd());
    shared_ptr<ConnectionEvent> event = conns[_c->get_fd()].second;
    if (event->onDisconnect) {
        event->onDisconnect(_c);
    }
};
void ConnectionPool::onSendBegin(shared_ptr<Connection> _c) {
    multiplexer->mod_fd(_c->get_fd(), true, true);
};
void ConnectionPool::onSendComplete(shared_ptr<Connection> _c) {
    multiplexer->mod_fd(_c->get_fd(), true, false);
    shared_ptr<ConnectionEvent> event = conns[_c->get_fd()].second;
    if (event->onSendComplete) {
        event->onSendComplete(_c);
    }
};
shared_ptr<Multiplexer> ConnectionPool::get_multiplexer() const {
    return multiplexer;
}