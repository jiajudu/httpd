#include "schedule/connectionPool.h"
#include "auxiliary/error.h"
#include "schedule/poller.h"
#include <iostream>
#include <poll.h>
#include <stdio.h>
ConnectionPool::ConnectionPool() {
    eh = make_shared<EventHandler>();
    eh->read = bind(&ConnectionPool::read_callback, this, _1);
    eh->write = bind(&ConnectionPool::write_callback, this, _1);
    eh->error = bind(&ConnectionPool::error_callback, this, _1);
    eh->close = bind(&ConnectionPool::close_callback, this, _1);
}
void ConnectionPool::add_connection(shared_ptr<Connection> connection,
                                    shared_ptr<ConnectionEvent> event) {
    conns[connection->get_fd()] = make_pair(connection, event);
    connection->pool = shared_from_this();
    multiplexer->add_fd(connection->get_fd(), true, false, eh);
    if (event->onConnection) {
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
void ConnectionPool::close_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd].first;
    conn->close();
}
void ConnectionPool::onClose(shared_ptr<Connection> _c) {
    shared_ptr<ConnectionEvent> event = conns[_c->get_fd()].second;
    conns.erase(_c->get_fd());
    multiplexer->del_fd(_c->get_fd());
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
