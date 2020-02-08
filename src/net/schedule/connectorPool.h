#pragma once
#include "net/schedule/eventhandler.h"
#include "net/service/service.h"
#include "net/socket/connection.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
class scheduler;
class ConnectorPool {
public:
    ConnectorPool();
    void connect(const string &ip, uint16_t port,
                 function<void(shared_ptr<Connection> conn)> onSuccess,
                 function<void(shared_ptr<Connection> conn)> onError);
    size_t size();
    Scheduler *scheduler;

private:
    void add_connection(shared_ptr<Connection> connection);
    shared_ptr<EventHandler> eh;
    class SocketInfo {
    public:
        shared_ptr<Socket> socket;
        function<void(shared_ptr<Connection> conn)> onSuccess;
        function<void(shared_ptr<Connection> conn)> onError;
    };
    unordered_map<int, SocketInfo> conns;
    void cannect_callback(int fd);
    void error_callback(int fd);
};
