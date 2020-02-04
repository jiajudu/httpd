#include "schedule/connectorPool.h"
#include "auxiliary/error.h"
#include "schedule/poller.h"
#include <poll.h>
#include <sys/socket.h>
ConnectorPool::ConnectorPool(shared_ptr<Multiplexer> _multiplexer)
    : multiplexer(_multiplexer) {
    if (multiplexer->connector_pool != 0) {
        fatal_error("Duplicated timer pool.");
    } else {
        multiplexer->connector_pool = shared_from_this();
    }
    eh = make_shared<EventHandler>();
    eh->write = bind(&ConnectorPool::cannect_callback, this, _1);
}
void ConnectorPool::connect(
    string &ip, uint16_t port,
    function<void(shared_ptr<Connection> conn)> onSuccess,
    function<void(shared_ptr<Connection> conn)> onError) {
    shared_ptr<Socket> socket = make_shared<Socket>(Socket::domain_INET);
    int ret = socket->connect(ip, port, true);
    if (ret == 0) {
        onSuccess(make_shared<Connection>(socket));
    } else {
        conns[socket->get_fd()] = {socket, onSuccess, onError};
        multiplexer->add_fd(socket->get_fd(), false, true, eh);
    }
}
void ConnectorPool::cannect_callback(int fd) {
    multiplexer->del_fd(fd);
    auto socket = conns[fd].socket;
    auto onSuccess = conns[fd].onSuccess;
    auto onError = conns[fd].onError;
    conns.erase(fd);
    int error = 0;
    socklen_t l = 0;
    int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &l);
    if (ret < 0) {
        syscall_error();
    }
    if (error == 0) {
        onSuccess(make_shared<Connection>(socket));
    } else {
        onError(make_shared<Connection>(socket));
    }
}
size_t ConnectorPool::size() {
    return conns.size();
}
shared_ptr<Multiplexer> ConnectorPool::get_multiplexer() const {
    return multiplexer;
}