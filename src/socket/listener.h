#pragma once
#include "auxiliary/std.h"
#include "socket/connection.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
class ListenerPool;
class Listener : enable_shared_from_this<Listener> {
public:
    Listener(string &ip, uint16_t port, int backlog);
    shared_ptr<Connection> accept();
    int close();
    int get_fd() const;
    function<void()> onClose = 0;
    shared_ptr<ListenerPool> pool;

private:
    shared_ptr<Socket> socket;
};