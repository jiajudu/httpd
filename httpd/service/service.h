#pragma once
#include "auxiliary/std.h"
#include "socket/connection.h"
#include <string>
class Service {
public:
    virtual void onConnection(shared_ptr<Connection> conn);
    virtual void onMessage(shared_ptr<Connection> conn, string &input_message);
    virtual void onSendComplete(shared_ptr<Connection> conn);
    function<size_t(char *buf, size_t n)> decoder;
};