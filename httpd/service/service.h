#pragma once
#include "auxiliary/std.h"
#include "socket/connection.h"
#include <string>
class Service {
public:
    virtual void onConnection(shared_ptr<Connection> conn) = 0;
    virtual void onMessage(shared_ptr<Connection> conn,
                           string &input_message) = 0;
    function<size_t(char *buf, size_t n)> decoder = 0;
};