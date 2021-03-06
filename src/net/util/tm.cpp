#include "net/util/tm.h"
#include "net/util/error.h"
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
string get_time_fmt_gmt() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t nowtime = tv.tv_sec;
    struct tm tm_time;
    gmtime_r(&nowtime, &tm_time);
    char buf[128];
    size_t len = strftime(buf, 128, "%c %Z", &tm_time);
    return string(buf, buf + len);
}
