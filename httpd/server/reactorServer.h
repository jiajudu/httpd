#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
class ReactorServer : public Server {
public:
    ReactorServer(string &ip, uint16_t port);
    void run();

private:
    string ip;
    uint16_t port;
    shared_ptr<Socket> listenSocket;
};