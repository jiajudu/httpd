#pragma once
#include "http/config.h"
#include "http/fastcgi.h"
#include "http/request.h"
#include "net/logging/logger.h"
#include "net/service/service.h"
#include "net/util/std.h"
#include <mutex>
#include <string>
#include <unordered_map>
class HTTP : public Service {
public:
    HTTP(HTTPDConfig &_config);
    void onConnection(shared_ptr<Connection> conn);
    void onMessage(shared_ptr<Connection> conn, string &input_message);
    void onSendComplete(shared_ptr<Connection> conn);
    void onDisconnect(shared_ptr<Connection> conn);
    void init(shared_ptr<Scheduler> scheduler);

private:
    void process_request(shared_ptr<Connection> conn);
    void process_file_request(shared_ptr<Connection> conn,
                              shared_ptr<HTTPRequest> r);
    void process_memory_request(shared_ptr<Connection> conn,
                                shared_ptr<HTTPRequest> r, const string& v);
    HTTPDConfig config;
    shared_ptr<FastCGI> fcgi;
    shared_ptr<Logger> logger;
    mutex lock;
    bool initialized = false;
};