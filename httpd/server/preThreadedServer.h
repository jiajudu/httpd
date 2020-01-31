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
    int numThreads;
    BlockingQueue<shared_ptr<Connection>> tasks;
    void worker_main();
};
