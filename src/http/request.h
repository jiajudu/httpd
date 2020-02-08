#pragma once
#include "net/util/std.h"
#include <memory>
#include <string>
#include <unordered_map>
class HTTPRequest {
public:
    unordered_map<string, string> kvs;
    string method;
    string uri;
    string path;
    string query_string;
    string protocol;
    string content;
};
class HTTPData {
public:
    int parse_status = 0;
    shared_ptr<HTTPRequest> r;
};