#include "auxiliary/std.h"
#include "multiplexer.h"
#include <memory>
#include <unordered_map>
class EventPool : enable_shared_from_this<EventPool> {
public:
    EventPool(shared_ptr<Multiplexer> _multiplexer);
    void add_event(int fd, function<void()> op, bool pre_read = true);
    void del_event(int fd);
    void event(int fd);
    shared_ptr<Multiplexer> get_multiplexer() const;

private:
    shared_ptr<Multiplexer> multiplexer;
    shared_ptr<EventHandler> eh;
    unordered_map<int, pair<bool, function<void()>>> ops;
};