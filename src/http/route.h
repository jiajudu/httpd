#pragma once
#include "net/util/std.h"
#include <string>
class Route {
public:
    // 0: startswith, 1: endswith, 2: all
    int type = 2;
    string pattern;
    // fastcgi, file
    string operation = "file";
    string host;
    uint16_t port;
};
bool match_route(const Route &route, const string &url);
