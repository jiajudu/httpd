#pragma once
#include "http/request.h"
#include "net/logging/logger.h"
#include "net/schedule/connectionPool.h"
#include "net/schedule/scheduler.h"
#include "net/util/std.h"
#include <vector>
#define FCGI_BEGIN_REQUEST 1
#define FCGI_ABORT_REQUEST 2
#define FCGI_END_REQUEST 3
#define FCGI_PARAMS 4
#define FCGI_STDIN 5
#define FCGI_STDOUT 6
#define FCGI_STDERR 7
#define FCGI_DATA 8
#define FCGI_GET_VALUES 9
#define FCGI_GET_VALUES_RESULT 10
#define FCGI_UNKNOWN_TYPE 11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)
struct FCGI_Header {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
};
struct FCGI_BeginRequestBody {
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
};
struct FCGI_EndRequestBody {
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
};
#define FCGI_RESPONDER 1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER 3
class FastCGITask {
public:
    void add_env(const string &key, const string &value);
    void add_content(const string &c);
    weak_ptr<HTTPRequest> request;
    vector<string> envs;
    vector<string> stdins;
    weak_ptr<Connection> http_conn;
    shared_ptr<Connection> fcgi_conn;
    Scheduler *scheduler = 0;
    string stdin;
    string stderr;
};
class FastCGI {
public:
    FastCGI(const string &_fcgi_host, uint16_t _fcgi_port, const string &_root,
            shared_ptr<Logger> _logger);
    void process_request(shared_ptr<Connection> conn,
                         shared_ptr<HTTPRequest> r);
    void interrupt_request(shared_ptr<Connection> conn,
                           shared_ptr<HTTPRequest> r);

private:
    const string fcgi_host;
    const uint16_t fcgi_port;
    const string root;
    unordered_map<string, string> header2env;
    mutex lock;
    unordered_map<uint16_t, shared_ptr<FastCGITask>> tasks;
    void set_header2env();
    void onConnectionEstablished(shared_ptr<Connection> conn, uint16_t id);
    void onConnectionError(shared_ptr<Connection> conn, uint16_t id);
    void onMessage(shared_ptr<Connection> conn, string &message, uint16_t id);
    void sendBeginRequest(shared_ptr<Connection> _conn, uint16_t id);
    void sendParams(shared_ptr<Connection> _conn, uint16_t id,
                    vector<string> &ps);
    void sendStdins(shared_ptr<Connection> _conn, uint16_t id,
                    vector<string> &ps);
    void sendInterrputRequest(shared_ptr<Connection> _conn, uint16_t id);
    size_t decode(char *s_buf, size_t n_buf);
    shared_ptr<Logger> logger;
};