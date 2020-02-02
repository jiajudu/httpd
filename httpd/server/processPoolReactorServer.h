#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/fdTransmission.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
class ProcessPoolReactorServer : public Server {
public:
    ProcessPoolReactorServer(shared_ptr<Service> _service, string &ip,
                             uint16_t port, ServerOption &server_option);
    void run();

private:
    void child_main(FDTransmission &fdt);
};
