#pragma once
#include "net/service/service.h"
#include "net/socket/listener.h"
#include "net/socket/socket.h"
#include "net/util/noncopyable.h"
#include <functional>
#include <memory>
#include <string>
class ServerOption {
public:
    int max_connection_number = 65536;
    int process_number = 16;
    int thread_number = 16;
    string scheduler = "epoll";
};
class Server {
public:
    Server(shared_ptr<Service> _service, string &_ip, uint16_t _port,
           ServerOption &_option)
        : service(_service), ip(_ip), port(_port), option(_option) {
    }
    virtual void run() = 0;

protected:
    shared_ptr<Service> service;
    string ip;
    uint16_t port;
    ServerOption option;
    shared_ptr<Listener> listener;
};
shared_ptr<Server> get_server(shared_ptr<Service> service, string &server_name,
                              string ip, uint16_t port, ServerOption &option);