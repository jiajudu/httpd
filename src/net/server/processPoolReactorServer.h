#pragma once
#include "net/server/server.h"
#include "net/socket/fdTransmission.h"
#include "net/socket/socket.h"
#include "net/util/noncopyable.h"
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
