#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "service/service.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <unordered_set>
class Chat : public Service {
public:
    void onConnection(shared_ptr<Connection> conn);
    void onMessage(shared_ptr<Connection> conn, string &input_message);
    void onDisconnect(shared_ptr<Connection> conn);

private:
    unordered_set<shared_ptr<Connection>> conns;
};