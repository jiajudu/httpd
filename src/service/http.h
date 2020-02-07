#pragma once
#include "auxiliary/std.h"
#include "http/fastcgi.h"
#include "service/service.h"
#include <string>
#include <unordered_map>
class HTTPRequest {
public:
    int parse_status = 0;
    unordered_map<string, string> kvs;
    string method;
    string uri;
    string path;
    string query_string;
    string protocol;
    string content;
};
class HTTP : public Service {
public:
    HTTP();
    void onConnection(shared_ptr<Connection> conn);
    void onMessage(shared_ptr<Connection> conn, string &input_message);
    void onSendComplete(shared_ptr<Connection> conn);
    void onDisconnect(shared_ptr<Connection> conn);

private:
    int parse_header(shared_ptr<Connection> conn, string &s);
    void error(shared_ptr<Connection> conn, int code);
    void process_request(shared_ptr<Connection> conn);
    string cwd;
    unordered_map<string, string> header2env;
    void set_header2env();
    const string REQUEST_METHOD = string("REQUEST_METHOD");
    const string QUERY_STRING = string("QUERY_STRING");
    const string SERVER_PROTOCOL = string("SERVER_PROTOCOL");
    const string SCRIPT_FILENAME = string("SCRIPT_FILENAME");
    const string CONTENT_TYPE = string("CONTENT_TYPE");
    const string CONTENT_LENGTH = string("CONTENT_LENGTH");
    const string SCRIPT_NAME = string("SCRIPT_NAME");
    const string REQUEST_URI = string("REQUEST_URI");
    const string DOCUMENT_URI = string("DOCUMENT_URI");
    const string DOCUMENT_ROOT = string("DOCUMENT_ROOT");
    const string GATEWAY_INTERFACE = string("GATEWAY_INTERFACE");
    const string SERVER_SOFTWARE = string("SERVER_SOFTWARE");
    const string REMOTE_ADDR = string("REMOTE_ADDR");
    const string REMOTE_PORT = string("REMOTE_PORT");
    const string SERVER_ADDR = string("SERVER_ADDR");
    const string SERVER_PORT = string("SERVER_PORT");
    const string SERVER_NAME = string("SERVER_NAME");
    const string REDIRECT_STATUS = string("REDIRECT_STATUS");
    const string REQUEST_SCHEME = string("REQUEST_SCHEME");
    const string FCGI_ROLE = string("FCGI_ROLE");
};