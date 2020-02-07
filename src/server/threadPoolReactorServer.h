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
    ThreadPoolReactorServer(shared_ptr<Service> _service, string &ip,
                            uint16_t port, ServerOption &server_option);
    void run();

private:
    void worker_main(Queue<shared_ptr<Connection>> &conn_q, int event_fd);
    int active_connection_number = 0;
    mutex lock;
    bool increase_connection_counter();
    bool decrease_connection_counter();
};