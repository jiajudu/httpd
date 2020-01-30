#pragma once
#include "auxiliary/noncopyable.h"
#include "socket/listener.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
class Server {
public:
    Server(string &_ip, uint16_t _port) : ip(_ip), port(_port) {
    }
    virtual void run() = 0;
    function<void(string &input_message,
                  function<size_t(string &output_message)> send)>
        onMessage;
    function<size_t(char *s, size_t n)> decoder;

protected:
    string ip;
    uint16_t port;
    shared_ptr<Listener> listener;
};