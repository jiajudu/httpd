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
    function<void(char *buf, ssize_t size,
                  function<ssize_t(char *buf, ssize_t size)> send)>
        onMessage;
};