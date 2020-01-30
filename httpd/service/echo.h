#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
class Echo {
public:
    Echo(shared_ptr<Server> server_);
    void onMessage(string &input_message,
                   function<size_t(string &output_message)> send);

private:
    shared_ptr<Server> server;
    size_t decode(char *buf, size_t n);
};