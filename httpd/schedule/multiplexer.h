#pragma once
#include "socket/connection.h"
class EventHandler {
public:
    function<void(int fd)> read = 0;
    function<void(int fd)> write = 0;
    function<void(int fd)> hang_up = 0;
    function<void(int fd)> error = 0;
};
class Multiplexer {
public:
    virtual void add_fd(int fd, bool event_read, bool event_write,
                        shared_ptr<EventHandler> eh) = 0;
    virtual void mod_fd(int fd, bool event_read, bool event_write) = 0;
    virtual void del_fd(int fd) = 0;
    virtual void read() = 0;
};