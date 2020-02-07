#pragma once
#include "net/server/server.h"
#include "net/socket/socket.h"
#include "net/util/noncopyable.h"
#include <functional>
#include <memory>
#include <string>
class ReactorServer : public Server {
public:
    ReactorServer(shared_ptr<Service> _service, string &ip, uint16_t port,
                  ServerOption &server_option);
    void run();
};