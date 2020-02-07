#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "service/service.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
class Echo : public Service {
public:
    void onMessage(shared_ptr<Connection> conn, string &input_message);
};