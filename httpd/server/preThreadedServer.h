#pragma once
#include "auxiliary/blockingQueue.h"
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
class PreThreadedServer : public Server {
public:
    PreThreadedServer(string &ip, uint16_t port, int numThreaded);
    void run();

private:
    string ip;
    uint16_t port;
    int numThreads;
    shared_ptr<Socket> listenSocket;
    BlockingQueue<shared_ptr<Socket>> tasks;
    void workerMain();
};
