#pragma once
#include "schedule/multiplexer.h"
#include "service/service.h"
#include "socket/connection.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
class ConnectorPool : enable_shared_from_this<ConnectorPool> {
public:
    ConnectorPool(shared_ptr<Multiplexer> _multiplexer);
    void connect(string &ip, uint16_t port,
                 function<void(shared_ptr<Connection> conn)> onSuccess,
                 function<void(shared_ptr<Connection> conn)> onError);
    size_t size();
    shared_ptr<Multiplexer> get_multiplexer() const;

private:
    void add_connection(shared_ptr<Connection> connection);
    shared_ptr<Multiplexer> multiplexer;
    shared_ptr<EventHandler> eh;
    class SocketInfo {
    public:
        shared_ptr<Socket> socket;
        function<void(shared_ptr<Connection> conn)> onSuccess;
        function<void(shared_ptr<Connection> conn)> onError;
    };
    unordered_map<int, SocketInfo> conns;
    void cannect_callback(int fd);
};
