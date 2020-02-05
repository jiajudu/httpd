#pragma once
#include "schedule/eventhandler.h"
#include "service/service.h"
#include "socket/connection.h"
#include <memory>
#include <unordered_map>
class Multiplexer;
class ConnectionEvent {
public:
    function<void(shared_ptr<Connection> conn)> onConnection = 0;
    function<void(shared_ptr<Connection> conn, string &message)> onMessage = 0;
    function<void(shared_ptr<Connection> conn)> onSendComplete = 0;
    function<void(shared_ptr<Connection> conn)> onDisconnect = 0;
};
class ConnectionPool {
public:
    ConnectionPool();
    void add_connection(shared_ptr<Connection> conn,
                        shared_ptr<ConnectionEvent> event);
    size_t size();
    void onClose(shared_ptr<Connection> conn);
    void onSendBegin(shared_ptr<Connection> conn);
    void onSendComplete(shared_ptr<Connection> conn);
    Multiplexer *multiplexer;

private:
    shared_ptr<EventHandler> eh;
    unordered_map<int,
                  pair<shared_ptr<Connection>, shared_ptr<ConnectionEvent>>>
        conns;
    void read_callback(int fd);
    void write_callback(int fd);
    void hang_up_callback(int fd);
    void error_callback(int fd);
};