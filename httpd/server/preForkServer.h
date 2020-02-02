#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/fdTransmission.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
class PreForkServer : public Server {
public:
    PreForkServer(shared_ptr<Service> _service, string &ip, uint16_t port,
                  ServerOption &server_option);
    void run();

private:
    vector<pair<FDTransmission, bool>> childs;
    void child_main(FDTransmission &fdt);
    size_t getAvailableProcess();
};
