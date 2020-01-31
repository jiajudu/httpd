#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "service/service.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
class Discard : public Service {
public:
    Discard();
    void onMessage(shared_ptr<Connection> conn, string &input_message);

private:
    size_t decode(char *s, size_t n);
};