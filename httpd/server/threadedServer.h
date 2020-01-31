#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
class ThreadedServer : public Server {
public:
    ThreadedServer(shared_ptr<Service> _service, string &ip, uint16_t port);
    void run();

private:
    void threadRun(shared_ptr<Connection> conn);
};