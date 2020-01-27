#pragma once
#include "auxiliary/noncopyable.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
using std::function;
class Server {
public:
    Server() {
    }
    virtual void run() = 0;
    virtual ssize_t send(shared_ptr<Socket> conn, char *buf, ssize_t size) = 0;
    function<void(char *buf, ssize_t size,
                  function<ssize_t(char *buf, ssize_t size)> send)>
        onMessage;
};