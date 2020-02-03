#include "schedule/clientConnectionPool.h"
#include "auxiliary/error.h"
#include "schedule/poller.h"
#include <poll.h>
#include <sys/socket.h>
ClientConnectionPool::ClientConnectionPool(shared_ptr<Multiplexer> _multiplexer)
    : multiplexer(_multiplexer) {
    eh = make_shared<EventHandler>();
    eh->read = bind(&ClientConnectionPool::read_callback, this, _1);
    eh->write = bind(&ClientConnectionPool::write_callback, this, _1);
    eh->error = bind(&ClientConnectionPool::error_callback, this, _1);
    eh->hang_up = bind(&ClientConnectionPool::hang_up_callback, this, _1);
    connector_eh = make_shared<EventHandler>();
    connector_eh->write =
        bind(&ClientConnectionPool::cannect_callback, this, _1);
}
void ClientConnectionPool::connect(string &ip, uint16_t port) {
    shared_ptr<Socket> socket = make_shared<Socket>(Socket::domain_INET);
    int ret = socket->connect(ip, port, true);
    if (ret == 0) {
        add_connection(make_shared<Connection>(socket));
    } else {
        establishing[socket->get_fd()] = socket;
        multiplexer->add_fd(socket->get_fd(), false, true, connector_eh);
    }
}
void ClientConnectionPool::cannect_callback(int fd) {
    multiplexer->del_fd(fd);
    shared_ptr<Socket> socket = establishing[fd];
    establishing.erase(fd);
    int error = 0;
    socklen_t l = 0;
    int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &l);
    if (ret < 0) {
        syscall_error();
    }
    if (error == 0) {
        establishing.erase(fd);
        multiplexer->del_fd(fd);
        add_connection(make_shared<Connection>(socket));
    } else {
        agreement_error("Connection error");
    }
}
void ClientConnectionPool::add_connection(shared_ptr<Connection> connection) {
    conns[connection->get_fd()] = connection;
    multiplexer->add_fd(connection->get_fd(), true, false, eh);
    connection->onClose = [this](shared_ptr<Connection> _c) -> void {
        conns.erase(_c->get_fd());
        multiplexer->del_fd(_c->get_fd());
        if (onDisconnect) {
            onDisconnect(_c);
        }
    };
    connection->onSendBegin = [this](shared_ptr<Connection> _c) -> void {
        multiplexer->mod_fd(_c->get_fd(), true, true);
    };
    connection->onSendComplete = [this](shared_ptr<Connection> _c) -> void {
        multiplexer->mod_fd(_c->get_fd(), true, false);
        if (onSendComplete) {
            onSendComplete(_c);
        }
    };
    if (onConnection) {
        onConnection(connection);
    }
}
size_t ClientConnectionPool::size() {
    return conns.size();
}
void ClientConnectionPool::read_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd];
    conn->non_blocking_recv();
    string message;
    conn->recv(message);
    if (message.size() > 0) {
        if (onMessage) {
            onMessage(conn, message);
        }
    }
}
void ClientConnectionPool::write_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd];
    conn->non_blocking_send();
}
void ClientConnectionPool::error_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd];
    multiplexer->del_fd(conn->get_fd());
    if (onDisconnect) {
        onDisconnect(conn);
    }
    conn->shutdown();
}
void ClientConnectionPool::hang_up_callback(int fd) {
    shared_ptr<Connection> conn = conns[fd];
    conn->close();
}
