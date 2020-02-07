#pragma once
#include "net/schedule/eventhandler.h"
#include "net/socket/connection.h"
#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
class Multiplexer;
class TimerPool {
public:
    TimerPool();
    void enable_deactivation();
    int set_timeout(function<void(void)> op, double seconds);
    void set_deactivation(shared_ptr<Connection> conn, int seconds);
    void cancel(int id);
    void event(int id);
    int get_sfd();
    Multiplexer *multiplexer;

private:
    shared_ptr<EventHandler> eh;
    unordered_map<int, function<void(void)>> timers;
    map<time_t, vector<weak_ptr<Connection>>> deactivations;
    int sfd = -1;
};