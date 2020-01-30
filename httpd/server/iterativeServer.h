#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
class IterativeServer : public Server {
public:
    IterativeServer(string &ip, uint16_t port);
    void run();
};