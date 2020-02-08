#include "net/logging/loggingstream.h"
#include "net/logging/logger.h"
#include "net/util/tm.h"
LoggingStream::LoggingStream(shared_ptr<Logger> _logger, int _level)
    : logger(_logger) {
    enable = (_level <= logger->get_level());
    if (enable) {
        ss << get_time_fmt() << ' ';
    }
}
LoggingStream &LoggingStream::operator<<(int number) {
    if (enable) {
        ss << number;
    }
    return *this;
}
LoggingStream &LoggingStream::operator<<(long number) {
    if (enable) {
        ss << number;
    }
    return *this;
}
LoggingStream &LoggingStream::operator<<(unsigned int number) {
    if (enable) {
        ss << number;
    }
    return *this;
}
LoggingStream &LoggingStream::operator<<(unsigned long number) {
    if (enable) {
        ss << number;
    }
    return *this;
}
LoggingStream &LoggingStream::operator<<(char c) {
    if (enable) {
        ss << c;
    }
    return *this;
}
LoggingStream &LoggingStream::operator<<(const string &str) {
    if (enable) {
        ss << str;
    }
    return *this;
}
LoggingStream::~LoggingStream() {
    if (enable) {
        logger->add_log(ss.str());
    }
}