#include "http/parser.h"
#include "http/error.h"
#include "http/request.h"
#include "net/util/std.h"
#include <string>
size_t http_decoder(char *s_buf, size_t n_buf, size_t max_len,
                    size_t expected_length) {
    if (expected_length == 0) {
        size_t len = min(n_buf, max_len);
        string s(s_buf, s_buf + len);
        size_t p = s.find("\r\n\r\n");
        if (p != string::npos) {
            return p + 4;
        }
        size_t q = s.find("\r\n\n");
        if (q != string::npos) {
            return q + 3;
        }
        return 0;
    } else {
        if (n_buf < expected_length) {
            return 0;
        } else {
            return expected_length;
        }
    }
}
vector<string> split(const string &s, char c = ' ') {
    vector<string> ret;
    string::const_iterator it = s.begin();
    while (it < s.end()) {
        string::const_iterator n = find(it, s.end(), c);
        if (it < n) {
            ret.push_back(string(it, n));
        }
        if (n == s.end()) {
            break;
        }
        it = n + 1;
    }
    for (auto &str : ret) {
        while (str.back() == ' ') {
            str.pop_back();
        }
        while (str.front() == ' ') {
            str.erase(str.begin());
        }
    }
    return ret;
}
vector<string> split2(const string &s, char c1 = '\r', char c2 = '\n') {
    vector<string> ret;
    string::const_iterator it = s.begin();
    while (it + 1 < s.end()) {
        string::const_iterator n = find(it, s.end(), c1);
        if (n + 1 < s.end() && *(n + 1) == c2) {
            if (it < n) {
                ret.push_back(string(it, n));
            }
        }
        if (n == s.end() || n + 1 == s.end()) {
            break;
        }
        it = n + 2;
    }
    for (auto &str : ret) {
        while (str.back() == ' ') {
            str.pop_back();
        }
        while (str.front() == ' ') {
            str.erase(str.begin());
        }
    }
    return ret;
}
int parse_header(shared_ptr<Connection> conn, string &s) {
    HTTPRequest &r = any_cast<HTTPRequest &>(conn->data);
    vector<string> lines = split2(s);
    vector<string> l1 = split(lines[0]);
    if (l1.size() != 2) {
        http_error(conn, 413);
        return -1;
    }
    r.method = l1[0];
    r.uri = l1[1];
    r.protocol = l1[2];
    vector<string> u2 = split(r.uri, '?');
    r.path = u2[0];
    if (r.path.size() == 0 || r.path[0] != '/') {
        http_error(conn, 400);
    }
    if (u2.size() >= 2) {
        r.query_string = u2[1];
    } else {
        r.query_string = "";
    }
    if (r.protocol != "HTTP/1.0" && r.protocol != "HTTP/1.1") {
        http_error(conn, 505);
        return -1;
    }
    for (size_t i = 1; i < lines.size(); i++) {
        vector<string> kv = split(lines[i], ':');
        if (kv[0].size() == 0 || kv[1].size() == 0) {
            http_error(conn, 400);
        }
        r.kvs[kv[0]] = kv[1];
    }
    return 0;
}