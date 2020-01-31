#pragma once
#include "auxiliary/blockingQueue.h"
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
#include <thread>
class ThreadPoolReactorServer : public Server {
public:
    ThreadPoolReactorServer(string &ip, uint16_t port, int numThreads);
    void run();

private:
    int numThreads;
    void worker_main(Queue<shared_ptr<Connection>> &conn_q, int event_fd);
};