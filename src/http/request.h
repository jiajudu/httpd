#pragma once
#include "net/util/std.h"
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