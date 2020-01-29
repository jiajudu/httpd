#pragma once
#include "auxiliary/noncopyable.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
class Server {
public:
    Server() {
    }
    virtual void run() = 0;
    function<void(vector<char> &buf, ssize_t size,
                  function<ssize_t(vector<char> &buf, ssize_t size)> send)>
        onMessage;
};