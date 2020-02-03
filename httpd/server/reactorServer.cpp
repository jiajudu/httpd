#include "server/reactorServer.h"
#include "auxiliary/error.h"
#include "auxiliary/tm.h"
#include "schedule/listenerPool.h"
#include "schedule/multiplexer.h"
#include "schedule/poller.h"
#include "schedule/timerPool.h"
#include <poll.h>
#include <unordered_set>
ReactorServer::ReactorServer(shared_ptr<Service> _service, string &_ip,
                             uint16_t _port, ServerOption &server_option)
    : Server(_service, _ip, _port, server_option) {
}
void ReactorServer::run() {
    shared_ptr<Multiplexer> multiplexer = make_shared<Poller>();
    listener = make_shared<Listener>(ip, port, 10);
    shared_ptr<ListenerPool> listener_pool =
        make_shared<ListenerPool>(multiplexer, listener);
    shared_ptr<ConnectionPool> connection_pool =
        make_shared<ConnectionPool>(multiplexer, service);
    shared_ptr<TimerPool> timer = make_shared<TimerPool>(multiplexer);
    listener_pool->onConnection = [connection_pool, this,
                                   timer](shared_ptr<Connection> conn) -> void {
        if (connection_pool->size() >=
            static_cast<size_t>(option.max_connection_number)) {
            conn->close();
        } else {
            conn->timer = timer;
            connection_pool->add_connection(conn);
        }
    };
    while (true) {
        multiplexer->read();
    }
}
