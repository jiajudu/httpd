#pragma once
#include "auxiliary/std.h"
#include <functional>
#include <memory>
class ConnectorPool;
class ConnectionPool;
class EventPool;
class ListenerPool;
class TimerPool;
class EventHandler {
public:
    function<void(int fd)> read = 0;
    function<void(int fd)> write = 0;
    function<void(int fd)> hang_up = 0;
    function<void(int fd)> error = 0;
};
class Multiplexer {
public:
    virtual void add_fd(int fd, bool event_read, bool event_write,
                        shared_ptr<EventHandler> eh) = 0;
    virtual void mod_fd(int fd, bool event_read, bool event_write) = 0;
    virtual void del_fd(int fd) = 0;
    virtual void read() = 0;
    shared_ptr<ConnectorPool> connector_pool;
    shared_ptr<ConnectionPool> connection_pool;
    shared_ptr<EventPool> event_pool;
    shared_ptr<ListenerPool> listener_pool;
    shared_ptr<TimerPool> timer_pool;
};