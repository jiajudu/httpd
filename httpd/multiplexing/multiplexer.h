#pragma once
#include "socket/connection.h"
#include "socket/listener.h"
class Multiplexer {
public:
    virtual void add_connection_fd(shared_ptr<Connection> connection,
                                   bool event_read, bool event_write) = 0;
    virtual void mod_connection_fd(shared_ptr<Connection> connection,
                                   bool event_read, bool event_write) = 0;
    virtual void del_connection_fd(shared_ptr<Connection> connection) = 0;
    virtual void add_event_fd(int fd) = 0;
    virtual void remove_event_fd(int fd) = 0;
    virtual void read() = 0;
    virtual size_t get_socket_number() = 0;
    function<void(shared_ptr<Connection> connection)> socket_read_callback = 0;
    function<void(shared_ptr<Connection> connection)> socket_write_callback = 0;
    function<void(shared_ptr<Connection> connection)> socket_hang_up_callback =
        0;
    function<void(shared_ptr<Connection> connection)> socket_error_callback = 0;
    function<void(int)> eventfd_read_callback = 0;
};