#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
using std::function;
class ThreadedServer : public Server {
public:
    ThreadedServer(string &ip, uint16_t port);
    void run();

private:
    string ip;
    uint16_t port;
    shared_ptr<Socket> listenSocket;
    void threadRun(shared_ptr<Socket> conn);
};