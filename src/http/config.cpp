#include "http/config.h"
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
            if (config.concurrence == "threadPoolReactor" ||
                config.concurrence == "processPoolReactor") {
                is >> config.concurrence_number;
            }
        } else if (key == "log") {
            is >> config.log;
        } else if (key == "timeout") {
            is >> config.timeout;
        } else if (key == "log_level") {
            string log_level;
            is >> log_level;
            if (log_level == "fatal") {
                config.log_level = 0;
            } else if (log_level == "error") {
                config.log_level = 1;
            } else if (log_level == "warn") {
                config.log_level = 2;
            } else if (log_level == "info") {
                config.log_level = 3;
            } else if (log_level == "debug") {
                config.log_level = 4;
            }
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
            } else if (type == "match") {
                route.type = 2;
                is >> route.pattern;
            } else {
                route.type = 3;
            }
            is >> route.operation;
            if (route.operation == "fastcgi") {
                is >> route.host;
                is >> route.port;
            } else if (route.operation == "memory") {
                is >> route.host;
            }
            config.routes.push_back(route);
        }
    }
    return config;
}