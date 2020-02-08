#pragma once
#include "net/util/std.h"
#include <memory>
#include <sstream>
#include <string>
class Logger;
class LoggingStream {
public:
    LoggingStream(shared_ptr<Logger> _logger, int _level);
    LoggingStream &operator<<(int number);
    LoggingStream &operator<<(long number);
    LoggingStream &operator<<(unsigned int number);
    LoggingStream &operator<<(unsigned long number);
    LoggingStream &operator<<(char c);
    LoggingStream &operator<<(const string &str);
    ~LoggingStream();

private:
    shared_ptr<Logger> logger;
    bool enable;
    stringstream ss;
};
#define LOG_FATAL LoggingStream(logger, 0)
#define LOG_ERROR LoggingStream(logger, 1)
#define LOG_WARN LoggingStream(logger, 2)
#define LOG_INFO LoggingStream(logger, 3)
#define LOG_DEBUG LoggingStream(logger, 4)
