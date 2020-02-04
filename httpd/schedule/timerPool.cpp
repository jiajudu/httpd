#include "schedule/timerPool.h"
#include "auxiliary/error.h"
#include "auxiliary/tm.h"
#include <sys/timerfd.h>
#include <unistd.h>
TimerPool::TimerPool(shared_ptr<Multiplexer> _multiplexer)
    : multiplexer(_multiplexer) {
    if (multiplexer->timer_pool != 0) {
        fatal_error("Duplicated timer pool.");
    } else {
        multiplexer->timer_pool = shared_from_this();
    }
    eh = make_shared<EventHandler>();
    sfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    struct itimerspec it;
    it.it_interval.tv_sec = 1;
    it.it_interval.tv_nsec = 0;
    it.it_value.tv_sec = 1;
    it.it_value.tv_nsec = 0;
    struct itimerspec tmp;
    timerfd_settime(sfd, 0, &it, &tmp);
    eh->read = bind(&TimerPool::event, this, _1);
    multiplexer->add_fd(sfd, true, false, eh);
}
int TimerPool::set_timeout(function<void(void)> op, double seconds) {
    int fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    struct itimerspec it;
    it.it_value.tv_sec = static_cast<long>(seconds);
    it.it_value.tv_nsec = static_cast<long>(
        (seconds - static_cast<double>(it.it_value.tv_sec)) * 1000000000);
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_nsec = 0;
    struct itimerspec tmp;
    timerfd_settime(fd, 0, &it, &tmp);
    timers[fd] = op;
    multiplexer->add_fd(fd, true, false, eh);
    return fd;
}
void TimerPool::set_deactivation(shared_ptr<Connection> conn, int seconds) {
    struct timeval tv = get_time();
    conn->deactivation_time = tv.tv_sec + seconds;
    deactivations[conn->deactivation_time].push_back(
        weak_ptr<Connection>(conn));
}
void TimerPool::cancel(int id) {
    if (timers.find(id) != timers.end()) {
        timers.erase(id);
        multiplexer->del_fd(id);
        close(id);
    }
}
void TimerPool::event(int id) {
    if (timers.find(id) != timers.end()) {
        char buf[8];
        read(id, buf, 8);
        timers[id]();
        timers.erase(id);
        multiplexer->del_fd(id);
        close(id);
    }
    if (id == sfd) {
        char buf[8];
        read(sfd, buf, 8);
        struct timeval tv = get_time();
        map<time_t, vector<weak_ptr<Connection>>>::iterator it;
        while (deactivations.size() > 0 &&
               ((it = deactivations.begin()), true) && it->first < tv.tv_sec) {
            for (weak_ptr<Connection> wp : it->second) {
                shared_ptr<Connection> sp = wp.lock();
                if (sp) {
                    if (sp->deactivation_time == it->first) {
                        sp->close();
                    }
                }
            }
            deactivations.erase(it->first);
        }
    }
}
int TimerPool::get_sfd() {
    return sfd;
}
shared_ptr<Multiplexer> TimerPool::get_multiplexer() const {
    return multiplexer;
}