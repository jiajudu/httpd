#include "net/logging/logger.h"
#include "net/util/error.h"
#include <chrono>
#include "net/util/tm.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
Logger::Logger(const string &_path, int _level) : level(_level) {
    fd = ::open(_path.c_str(), O_WRONLY | O_APPEND | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        syscall_error();
    }
    t = move(thread(&Logger::thread_func, this));
}
Logger::~Logger() {
    exit = true;
    t.join();
}
void Logger::add_log(const string &log) {
    lock_guard<mutex> g(lock);
    buf_input.write(log.c_str(), log.size());
    cv.notify_all();
}
int Logger::get_level() const {
    return level;
}
void Logger::set_level(int _level) {
    level = _level;
}
void Logger::thread_func() {
    while (true) {
        {
            unique_lock<mutex> g(lock);
            while (buf_input.size() == 0) {
                if (exit) {
                    return;
                }
                cv.wait_for(g, 1s);
            }
            buf_input.swap_buf(buf_output);
        }
        while (buf_output.size() > 0) {
            buf_output.read([this](char *s, size_t n) -> size_t {
                ssize_t ret = write(fd, s, n);
                if (ret > 0) {
                    return ret;
                } else {
                    return 0;
                }
            });
        }
    }
    cout << get_time_fmt() << endl;
}