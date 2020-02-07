#include "net/server/reactorServer.h"
#include "http/fastcgi.h"
#include "net/schedule/connectionPool.h"
#include "net/schedule/listenerPool.h"
#include "net/schedule/multiplexer.h"
#include "net/schedule/poller.h"
#include "net/schedule/timerPool.h"
#include "net/util/error.h"
#include "net/util/tm.h"
#include <poll.h>
#include <unordered_set>
ReactorServer::ReactorServer(shared_ptr<Service> _service, string &_ip,
                             uint16_t _port, ServerOption &server_option)
    : Server(_service, _ip, _port, server_option) {
}
void ReactorServer::run() {
    shared_ptr<Multiplexer> multiplexer = make_shared<Poller>();
    shared_ptr<ConnectionPool> connection_pool = multiplexer->connections;
    shared_ptr<ListenerPool> listener_pool = multiplexer->listeners;
    listener = make_shared<Listener>(ip, port, 10);
    service->init(multiplexer);
    shared_ptr<ConnectionEvent> conn_ev = make_shared<ConnectionEvent>();
    conn_ev->onConnection = [this](shared_ptr<Connection> conn) -> void {
        service->onConnection(conn);
    };
    conn_ev->onMessage = [this](shared_ptr<Connection> conn,
                                string &message) -> void {
        service->onMessage(conn, message);
    };
    conn_ev->onSendComplete = [this](shared_ptr<Connection> conn) -> void {
        service->onSendComplete(conn);
    };
    conn_ev->onDisconnect = [this](shared_ptr<Connection> conn) -> void {
        service->onDisconnect(conn);
    };
    listener_pool->add_listener(
        listener,
        [&connection_pool, this,
         &conn_ev](shared_ptr<Connection> conn) -> void {
            if (connection_pool->size() >=
                static_cast<size_t>(option.max_connection_number)) {
                conn->close();
            } else {
                connection_pool->add_connection(conn, conn_ev);
            }
        });
    while (true) {
        multiplexer->read();
    }
}
