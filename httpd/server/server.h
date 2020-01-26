#pragma once
#include "auxiliary/noncopyable.h"
#include "socket/socket.h"
#include <memory>
#include <string>
class Server : noncopyable {
public:
    Server(std::string &ip, uint16_t port);
    void run();

private:
    std::shared_ptr<Socket> listenSocket;
};