#include "http/config.h"
// listen 1234
// index index.html
// route end .php fastcgi 127.0.0.1 8000
// route all file
HTTPDConfig parse_http_config(const string &file) {
    ifstream is(file);
    HTTPDConfig config;
    while (is) {
        string key;
        is >> key;
        if (key == "listen") {
            is >> config.port;
        } else if (key == "index") {
            is >> config.index;
        } else if (key == "root") {
            is >> config.root;
        } else if (key == "concurrence") {
            is >> config.concurrence;
        } else if (key == "route") {
            Route route;
            string type;
            is >> type;
            if (type == "begin") {
                route.type = 0;
                is >> route.pattern;
            } else if (type == "end") {
                route.type = 1;
                is >> route.pattern;
            } else {
                route.type = 2;
            }
            is >> route.operation;
            if (route.operation == "fastcgi") {
                is >> route.host;
                is >> route.port;
            }
            config.routes.push_back(route);
        }
    }
    return config;
}