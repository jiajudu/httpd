#pragma once
#include "schedule/multiplexer.h"
#include "service/service.h"
#include "socket/connection.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
class ClientConnectionPool {
public:
    ClientConnectionPool(shared_ptr<Multiplexer> _multiplexer);
    void connect(string &ip, uint16_t port);
    size_t size();
    function<void(shared_ptr<Connection> conn)> onConnection;
    function<void(shared_ptr<Connection> conn, string& message)> onMessage;
    function<void(shared_ptr<Connection> conn)> onSendComplete;
    function<void(shared_ptr<Connection> conn)> onDisconnect;

private:
    void add_connection(shared_ptr<Connection> connection);
    shared_ptr<Multiplexer> multiplexer;
    shared_ptr<EventHandler> eh;
    shared_ptr<EventHandler> connector_eh;
    unordered_map<int, shared_ptr<Connection>> conns;
    unordered_map<int, shared_ptr<Socket>> establishing;
    void cannect_callback(int fd);
    void read_callback(int fd);
    void write_callback(int fd);
    void hang_up_callback(int fd);
    void error_callback(int fd);
};
