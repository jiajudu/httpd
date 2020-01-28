#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
class Echo {
public:
    Echo(shared_ptr<Server> server_);
    void onMessage(char *buf, ssize_t size,
                   function<ssize_t(char *buf, ssize_t size)> send);

private:
    shared_ptr<Server> server;
};