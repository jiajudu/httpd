#pragma once
#include "net/schedule/eventhandler.h"
#include "net/socket/connection.h"
#include "net/socket/listener.h"
#include "net/socket/socket.h"
#include "net/util/std.h"
#include <memory>
#include <string>
#include <unordered_map>
class Scheduler;
class ListenerPool : public enable_shared_from_this<ListenerPool> {
public:
    ListenerPool();
    void add_listener(shared_ptr<Listener> listener,
                      function<void(shared_ptr<Connection>)> onConnection);
    void remove_listener(shared_ptr<Listener> listener);
    Scheduler *scheduler;

private:
    unordered_map<int, pair<shared_ptr<Listener>,
                            function<void(shared_ptr<Connection> conn)>>>
        listeners;
    shared_ptr<EventHandler> eh;
    void read_callback(int fd);
};