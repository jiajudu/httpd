#pragma once
#include "auxiliary/std.h"
#include "schedule/multiplexer.h"
#include "socket/connection.h"
#include "socket/listener.h"
#include "socket/socket.h"
#include <memory>
#include <string>
#include <unordered_map>
class ListenerPool : enable_shared_from_this<ListenerPool> {
public:
    ListenerPool(shared_ptr<Multiplexer> _multiplexer);
    void add_listener(shared_ptr<Listener> listener,
                      function<void(shared_ptr<Connection>)> onConnection);
    void remove_listener(shared_ptr<Listener> listener);
    shared_ptr<Multiplexer> get_multiplexer() const;

private:
    shared_ptr<Multiplexer> multiplexer;
    unordered_map<int, pair<shared_ptr<Listener>,
                            function<void(shared_ptr<Connection> conn)>>>
        listeners;
    shared_ptr<EventHandler> eh;
    void read_callback(int fd);
};