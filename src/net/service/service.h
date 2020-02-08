#pragma once
#include "net/socket/connection.h"
#include "net/util/std.h"
#include "net/util/tm.h"
#include <any>
#include <string>
class Scheduler;
class Service {
public:
    virtual void onConnection(shared_ptr<Connection> conn);
    virtual void onMessage(shared_ptr<Connection> conn, string &input_message);
    virtual void onSendComplete(shared_ptr<Connection> conn);
    virtual void onDisconnect(shared_ptr<Connection> conn);
    virtual void init(shared_ptr<Scheduler> scheduler);
};