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
    multiplexer->add_event_fd(listener->get_fd());
    multiplexer->eventfd_read_callback = [&](int _fd) -> void {
        if (listener->get_fd() == _fd) {
            shared_ptr<Connection> conn = listener->accept();
            multiplexer->add_connection_fd(conn, true, false);
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
        multiplexer->mod_connection_fd(conn, true, conn->has_content_to_send());
    };
    multiplexer->socket_write_callback =
        [&](shared_ptr<Connection> conn) -> void {
        conn->non_blocking_send();
        multiplexer->mod_connection_fd(conn, true, conn->has_content_to_send());
    };
    multiplexer->socket_error_callback =
        [&](shared_ptr<Connection> conn) -> void {
        multiplexer->del_connection_fd(conn);
        conn->close();
    };
    multiplexer->socket_hang_up_callback =
        [&](shared_ptr<Connection> conn) -> void {
        multiplexer->del_connection_fd(conn);
        conn->close();
    };
    while (true) {
        multiplexer->read();
    }
}
