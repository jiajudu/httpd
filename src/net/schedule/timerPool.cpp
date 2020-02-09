#include "net/schedule/timerPool.h"
#include "net/schedule/scheduler.h"
#include "net/util/error.h"
#include "net/util/tm.h"
#include <sys/timerfd.h>
#include <unistd.h>
TimerPool::TimerPool() {
    eh = make_shared<EventHandler>();
    eh->read = bind(&TimerPool::event, this, _1);
}
void TimerPool::enable_deactivation() {
    sfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    struct itimerspec it;
    it.it_interval.tv_sec = 1;
    it.it_interval.tv_nsec = 0;
    it.it_value.tv_sec = 1;
    it.it_value.tv_nsec = 0;
    struct itimerspec tmp;
    timerfd_settime(sfd, 0, &it, &tmp);
    scheduler->add_fd(sfd, true, false, eh);
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
    scheduler->add_fd(fd, true, false, eh);
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
        scheduler->del_fd(id);
        close(id);
    }
}
void TimerPool::event(int id) {
    if (timers.find(id) != timers.end()) {
        char buf[8];
        ssize_t ret = read(id, buf, 8);
        if (ret < 0) {
            syscall_error();
        }
        timers[id]();
        timers.erase(id);
        scheduler->del_fd(id);
        close(id);
    }
    if (id == sfd) {
        char buf[8];
        ssize_t ret = read(sfd, buf, 8);
        if (ret < 0) {
            syscall_error();
        }
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
