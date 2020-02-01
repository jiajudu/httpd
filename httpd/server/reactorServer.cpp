#include "server/reactorServer.h"
#include "auxiliary/error.h"
#include "multiplexing/multiplexer.h"
#include "multiplexing/poller.h"
#include <poll.h>
#include <unordered_set>
ReactorServer::ReactorServer(shared_ptr<Service> _service, string &_ip,
                             uint16_t _port)
    : Server(_service, _ip, _port) {
}
void ReactorServer::run() {
    listener = make_shared<Listener>(ip, port, 10, true);
    shared_ptr<Multiplexer> multiplexer = make_shared<Poller>();
    auto connection_close = [&](shared_ptr<Connection> conn) -> void {
        multiplexer->del_connection_fd(conn);
    };
    auto connection_send_begin = [&](shared_ptr<Connection> conn) -> void {
        multiplexer->mod_connection_fd(conn, true, true);
    };
    auto connection_send_end = [&](shared_ptr<Connection> conn) -> void {
        multiplexer->mod_connection_fd(conn, true, false);
        service->onSendComplete(conn);
    };
    multiplexer->add_event_fd(listener->get_fd());
    multiplexer->eventfd_read_callback = [&](int _fd) -> void {
        if (listener->get_fd() == _fd) {
            shared_ptr<Connection> conn = listener->accept();
            multiplexer->add_connection_fd(conn, true, false);
            conn->onClose = connection_close;
            conn->onSendBegin = connection_send_begin;
            conn->onSendComplete = connection_send_end;
            service->onConnection(conn);
        }
    };
    multiplexer->socket_read_callback =
        [&](shared_ptr<Connection> conn) -> void {
        conn->non_blocking_recv();
        string message;
        conn->recv(message, service->decoder);
        if (message.size() > 0) {
            service->onMessage(conn, message);
        }
    };
    multiplexer->socket_write_callback =
        [&](shared_ptr<Connection> conn) -> void { conn->non_blocking_send(); };
    multiplexer->socket_error_callback =
        [&](shared_ptr<Connection> conn) -> void {
        multiplexer->del_connection_fd(conn);
        conn->shutdown();
    };
    multiplexer->socket_hang_up_callback =
        [&](shared_ptr<Connection> conn) -> void { conn->close(); };
    while (true) {
        multiplexer->read();
    }
}
