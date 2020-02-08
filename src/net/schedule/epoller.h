#pragma once
#include "net/schedule/scheduler.h"
#include <unordered_set>
class EPoller : public Scheduler {
public:
    EPoller();
    virtual void add_fd(int fd, bool event_read, bool event_write,
                        shared_ptr<EventHandler> eh);
    virtual void mod_fd(int fd, bool event_read, bool event_write);
    virtual void del_fd(int fd);
    void read();

private:
    int epoll_fd = 0;
    unordered_map<int, pair<short, shared_ptr<EventHandler>>> fds;
};