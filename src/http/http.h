#pragma once
#include "http/config.h"
#include "http/fastcgi.h"
#include "http/request.h"
#include "net/logging/logger.h"
#include "net/service/service.h"
#include "net/util/std.h"
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
    int parse_header(shared_ptr<Connection> conn, string &s);
    void process_request(shared_ptr<Connection> conn);
    void process_file_request(shared_ptr<Connection> conn, HTTPRequest &r);
    HTTPDConfig config;
    shared_ptr<FastCGI> &fcgi();
    shared_ptr<Logger> logger;
};