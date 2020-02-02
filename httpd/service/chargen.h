#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "service/service.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
class Chargen : public Service {
public:
    Chargen();
    void onConnection(shared_ptr<Connection> conn);
    void onSendComplete(shared_ptr<Connection> conn);

private:
    string chars;
};