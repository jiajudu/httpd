#pragma once
#include "auxiliary/std.h"
#include "socket/connection.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
class Listener {
public:
    Listener(string &ip, uint16_t port, int backlog);
    shared_ptr<Connection> accept();
    int close();
    int get_fd() const;
    function<void()> onClose = 0;

private:
    shared_ptr<Socket> socket;
};