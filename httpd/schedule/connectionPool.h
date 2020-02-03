#pragma once
#include "schedule/multiplexer.h"
#include "service/service.h"
#include "socket/connection.h"
#include <memory>
#include <unordered_map>
class ConnectionPool {
public:
    ConnectionPool(shared_ptr<Multiplexer> _multiplexer,
                   shared_ptr<Service> _service);
    void add_connection(shared_ptr<Connection> conn);
    size_t size();

private:
    shared_ptr<Multiplexer> multiplexer;
    shared_ptr<Service> service;
    shared_ptr<EventHandler> eh;
    unordered_map<int, shared_ptr<Connection>> conns;
    void read_callback(int fd);
    void write_callback(int fd);
    void hang_up_callback(int fd);
    void error_callback(int fd);
};