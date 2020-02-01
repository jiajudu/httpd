#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "service/service.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
class Echo : public Service {
public:
    Echo();
    void onMessage(shared_ptr<Connection> conn, string &input_message);
    void onConnection(shared_ptr<Connection> conn);

private:
    size_t decode(char *buf, size_t n);
};