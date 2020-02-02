#include "auxiliary/tm.h"
#include "auxiliary/error.h"
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
struct timeval get_time() {
    struct timeval tv;
    int ret = gettimeofday(&tv, 0);
    if (ret < 0) {
        syscall_error();
    }
    return tv;
}
string get_time_fmt() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t nowtime = tv.tv_sec;
    struct tm tm_time;
    gmtime_r(&nowtime, &tm_time);
    char buf[128];
    int len =
        snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d.%06ld",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, tv.tv_usec);
    if (len < 0) {
        len = 0;
    }
    return string(buf, buf + len);
}
Timer::Timer() {
    sfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    struct itimerspec it;
    it.it_interval.tv_sec = 1;
    it.it_interval.tv_nsec = 0;
    it.it_value.tv_sec = 1;
    it.it_value.tv_nsec = 0;
    struct itimerspec tmp;
    timerfd_settime(sfd, 0, &it, &tmp);
}
int Timer::set_timeout(function<void(void)> op, double seconds) {
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
    if (add_timer_callback) {
        add_timer_callback(fd);
    }
    return fd;
}
void Timer::set_deactivation(shared_ptr<Connection> conn, int seconds) {
    struct timeval tv = get_time();
    conn->deactivation_time = tv.tv_sec + seconds;
    deactivations[conn->deactivation_time].push_back(
        weak_ptr<Connection>(conn));
}
void Timer::cancel(int id) {
    if (timers.find(id) != timers.end()) {
        timers.erase(id);
        close(id);
        if (remove_timer_callback) {
            remove_timer_callback(id);
        }
    }
}
void Timer::event(int id) {
    if (timers.find(id) != timers.end()) {
        char buf[8];
        read(id, buf, 8);
        timers[id]();
        timers.erase(id);
        close(id);
        if (remove_timer_callback) {
            remove_timer_callback(id);
        }
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
int Timer::get_sfd() {
    return sfd;
}