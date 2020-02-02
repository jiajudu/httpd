#include "server/reactorServer.h"
#include "auxiliary/error.h"
#include "auxiliary/tm.h"
#include "multiplexing/multiplexer.h"
#include "multiplexing/poller.h"
#include <poll.h>
#include <unordered_set>
ReactorServer::ReactorServer(shared_ptr<Service> _service, string &_ip,
                             uint16_t _port, ServerOption &server_option)
    : Server(_service, _ip, _port, server_option) {
}
void ReactorServer::run() {
    listener = make_shared<Listener>(ip, port, 10);
    shared_ptr<Multiplexer> multiplexer = make_shared<Poller>();
    multiplexer->add_event_fd(listener->get_fd());
    shared_ptr<Timer> timer = make_shared<Timer>();
    timer->add_timer_callback = [&](int fd) -> void {
        multiplexer->add_event_fd(fd);
    };
    timer->remove_timer_callback = [&](int fd) -> void {
        multiplexer->remove_event_fd(fd);
    };
    multiplexer->add_event_fd(timer->get_sfd());
    multiplexer->eventfd_read_callback = [&](int _fd) -> void {
        if (listener->get_fd() == _fd) {
            shared_ptr<Connection> conn = listener->accept();
            if (multiplexer->get_socket_number() >=
                static_cast<size_t>(option.max_connection_number)) {
                conn->close();
            } else {
                conn->timer = timer;
                multiplexer->add_connection_fd(conn, true, false);
                conn->onClose = [&](shared_ptr<Connection> _c) -> void {
                    multiplexer->del_connection_fd(_c);
                    service->onDisconnect(_c);
                };
                conn->onSendBegin = [&](shared_ptr<Connection> _c) -> void {
                    multiplexer->mod_connection_fd(_c, true, true);
                };
                conn->onSendComplete = [&](shared_ptr<Connection> _c) -> void {
                    multiplexer->mod_connection_fd(_c, true, false);
                    service->onSendComplete(_c);
                };
                service->onConnection(conn);
            }
        } else {
            timer->event(_fd);
        }
    };
    multiplexer->socket_read_callback =
        [&](shared_ptr<Connection> conn) -> void {
        conn->non_blocking_recv();
        string message;
        conn->recv(message);
        if (message.size() > 0) {
            service->onMessage(conn, message);
        }
    };
    multiplexer->socket_write_callback =
        [&](shared_ptr<Connection> conn) -> void { conn->non_blocking_send(); };
    multiplexer->socket_error_callback =
        [&](shared_ptr<Connection> conn) -> void {
        multiplexer->del_connection_fd(conn);
        service->onDisconnect(conn);
        conn->shutdown();
    };
    multiplexer->socket_hang_up_callback =
        [&](shared_ptr<Connection> conn) -> void { conn->close(); };
    while (true) {
        multiplexer->read();
    }
}
