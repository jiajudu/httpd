#include "http/route.h"
bool match_route(const Route &route, const string &url) {
    if (route.type == 0) {
        return url.find(route.pattern) == 0;
    } else if (route.type == 1) {
        return url.rfind(route.pattern) == url.size() - route.pattern.size();
    } else if (route.type == 2) {
        return route.pattern == url;
    } else if (route.type == 3) {
        return true;
    } else {
        return false;
    }
}