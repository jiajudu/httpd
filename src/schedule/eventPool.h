#pragma once
#include "auxiliary/std.h"
#include "schedule/eventhandler.h"
#include <memory>
#include <unordered_map>
class Multiplexer;
class EventPool {
public:
    EventPool();
    void add_event(int fd, function<void()> op, bool pre_read = true);
    void del_event(int fd);
    void event(int fd);
    Multiplexer *multiplexer;

private:
    shared_ptr<EventHandler> eh;
    unordered_map<int, pair<bool, function<void()>>> ops;
};