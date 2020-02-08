#pragma once
#include "net/util/buffer.h"
#include "net/util/std.h"
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
class Logger {
public:
    Logger(const string &_path, int _level = 3);
    void add_log(const string &log);
    int get_level() const;
    void set_level(int _level);

private:
    int fd;
    // 0: fatal, 1: error, 2: warn, 3: info, 4: debug
    int level;
    mutex lock;
    condition_variable cv;
    Buffer buf_input;
    Buffer buf_output;
    thread t;
    void thread_func();
};