#include "schedule/eventPool.h"
#include "auxiliary/error.h"
#include "schedule/multiplexer.h"
#include <sys/eventfd.h>
EventPool::EventPool() {
    eh = make_shared<EventHandler>();
    eh->read = bind(&EventPool::event, this, _1);
}
void EventPool::add_event(int fd, function<void()> op, bool pre_read) {
    multiplexer->add_fd(fd, true, false, eh);
    ops[fd] = {pre_read, op};
}
void EventPool::del_event(int fd) {
    multiplexer->del_fd(fd);
    ops.erase(fd);
}
void EventPool::event(int fd) {
    if (ops.find(fd) != ops.end()) {
        if (ops[fd].first) {
            eventfd_t e;
            eventfd_read(ops[fd].first, &e);
        }
        ops[fd].second();
    }
}
