#include "schedule/multiplexer.h"
#include "socket/connection.h"
#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
class TimerPool : enable_shared_from_this<TimerPool> {
public:
    TimerPool(shared_ptr<Multiplexer> _multiplexer);
    int set_timeout(function<void(void)> op, double seconds);
    void set_deactivation(shared_ptr<Connection> conn, int seconds);
    void cancel(int id);
    void event(int id);
    int get_sfd();
    shared_ptr<Multiplexer> get_multiplexer() const;

private:
    shared_ptr<Multiplexer> multiplexer;
    shared_ptr<EventHandler> eh;
    unordered_map<int, function<void(void)>> timers;
    map<time_t, vector<weak_ptr<Connection>>> deactivations;
    int sfd;
};