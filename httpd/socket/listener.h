#pragma once
#include "auxiliary/std.h"
#include "socket/connection.h"
#include "socket/socket.h"
#include <memory>
#include <string>
class Listener {
public:
    Listener(string &ip, uint16_t port, int backlog,
             bool is_non_blocking = false);
    shared_ptr<Connection> accept();
    int close();
    int get_fd() const;

private:
    shared_ptr<Socket> socket;
    bool is_non_blocking;
};