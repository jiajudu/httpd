#pragma once
#include "auxiliary/std.h"
#include "schedule/connectionPool.h"
#include "schedule/multiplexer.h"
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
    vector<string> &get_envs();
    vector<string> &get_stdins();

private:
    vector<string> envs;
    vector<string> stdins;
};
class FastCGI {
public:
    FastCGI(string &ip, uint16_t address, shared_ptr<Multiplexer> _m);
    void submit(FastCGITask task, shared_ptr<Connection> _c);
    bool available() const;

private:
    bool is_available = false;
    shared_ptr<Multiplexer> m;
    shared_ptr<Connection> conn;
    uint16_t counter = 0;
    class Result {
    public:
        shared_ptr<Connection> conn;
        string stdin;
        string stderr;
    };
    unordered_map<uint16_t, Result> tasks;
    void onConnectionEstablished(shared_ptr<Connection> conn);
    void onConnectionError(shared_ptr<Connection> conn);
    void onMessage(shared_ptr<Connection> conn, string &message);
    void sendBeginRequest(uint16_t id);
    void sendParams(uint16_t id, vector<string> &ps);
    void sendStdins(uint16_t id, vector<string> &ps);
    size_t decode(char *s_buf, size_t n_buf);
};