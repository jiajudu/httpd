#pragma once
#include "auxiliary/std.h"
#include "socket/connection.h"
#include <map>
#include <mutex>
#include <string>
#include <sys/time.h>
#include <unordered_map>
#include <vector>
struct timeval get_time();
string get_time_fmt();
class Timer {
public:
    Timer();
    int set_timeout(function<void(void)> op, double seconds);
    void set_deactivation(shared_ptr<Connection> conn, int seconds);
    void cancel(int id);
    function<void(int)> add_timer_callback = 0;
    function<void(int)> remove_timer_callback = 0;
    void event(int id);
    int get_sfd();

private:
    unordered_map<int, function<void(void)>> timers;
    map<time_t, vector<weak_ptr<Connection>>> deactivations;
    int sfd;
};