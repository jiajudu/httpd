#include "schedule/multiplexer.h"
#include <unordered_set>
class Poller : public Multiplexer {
public:
    virtual void add_fd(int fd, bool event_read, bool event_write,
                        shared_ptr<EventHandler> eh);
    virtual void mod_fd(int fd, bool event_read, bool event_write);
    virtual void del_fd(int fd);
    void read();

private:
    unordered_map<int, pair<short, shared_ptr<EventHandler>>> fds;
};