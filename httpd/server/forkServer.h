#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
class ForkServer : public Server {
public:
    ForkServer(shared_ptr<Service> _service, string &ip, uint16_t port, ServerOption& server_option);
    void run();
};