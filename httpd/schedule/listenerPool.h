#pragma once
#include "auxiliary/std.h"
#include "schedule/connectionPool.h"
#include "schedule/multiplexer.h"
#include "socket/connection.h"
#include "socket/listener.h"
#include "socket/socket.h"
#include <memory>
#include <string>
class ListenerPool {
public:
    ListenerPool(shared_ptr<Multiplexer> _multiplexer,
                 shared_ptr<Listener> _listener);
    function<void(shared_ptr<Connection>)> onConnection = 0;

private:
    shared_ptr<Multiplexer> multiplexer;
    shared_ptr<Listener> listener;
    shared_ptr<EventHandler> eh;
};