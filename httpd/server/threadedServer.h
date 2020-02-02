#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
class ThreadedServer : public Server {
public:
    ThreadedServer(shared_ptr<Service> _service, string &ip, uint16_t port,
                   ServerOption &server_option);
    void run();

private:
    void threadRun(shared_ptr<Connection> conn);
    int active_connection_number = 0;
    mutex lock;
    bool increase_connection_counter();
    bool decrease_connection_counter();
};