#pragma once
#include "http/route.h"
#include "net/util/std.h"
#include <fstream>
#include <limits.h>
#include <string>
#include <vector>
class HTTPDConfig {
public:
    string concurrence = "reactor";
    int concurrence_number = 4;
    uint16_t port = 80;
    string log = "/var/log/httpd.log";
    int log_level = 3;
    int timeout = 10;
    string root = "/var/www/html";
    string index = "index.html";
    vector<Route> routes;
};
HTTPDConfig parse_http_config(const string &file);
