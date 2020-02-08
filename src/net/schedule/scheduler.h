#pragma once
#include "net/schedule/connectionPool.h"
#include "net/schedule/connectorPool.h"
#include "net/schedule/eventPool.h"
#include "net/schedule/listenerPool.h"
#include "net/schedule/timerPool.h"
#include "net/util/std.h"
#include <functional>
#include <memory>
class Scheduler {
public:
    Scheduler();
    virtual void add_fd(int fd, bool event_read, bool event_write,
                        shared_ptr<EventHandler> eh) = 0;
    virtual void mod_fd(int fd, bool event_read, bool event_write) = 0;
    virtual void del_fd(int fd) = 0;
    virtual void read() = 0;
    const shared_ptr<ListenerPool> listeners;
    const shared_ptr<ConnectorPool> connectors;
    const shared_ptr<ConnectionPool> connections;
    const shared_ptr<EventPool> events;
    const shared_ptr<TimerPool> timers;
};