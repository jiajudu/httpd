#include "multiplexer.h"
#include <unordered_map>
class EventPool {
public:
    EventPool(shared_ptr<Multiplexer> _multiplexer);
    void add_event(int fd, function<void()> op, bool pre_read = true);
    void del_event(int fd);
    void event(int fd);

private:
    shared_ptr<Multiplexer> multiplexer;
    shared_ptr<EventHandler> eh;
    unordered_map<int, pair<bool, function<void()>>> ops;
};