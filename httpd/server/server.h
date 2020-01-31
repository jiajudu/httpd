#pragma once
#include "auxiliary/noncopyable.h"
#include "service/service.h"
#include "socket/listener.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
class Server {
public:
    Server(shared_ptr<Service> _service, string &_ip, uint16_t _port)
        : service(_service), ip(_ip), port(_port) {
    }
    virtual void run() = 0;

protected:
    shared_ptr<Service> service;
    string ip;
    uint16_t port;
    shared_ptr<Listener> listener;
};