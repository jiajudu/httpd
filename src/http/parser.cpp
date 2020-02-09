#include "http/parser.h"
#include "http/error.h"
#include "http/request.h"
#include "net/util/std.h"
#include "net/util/tm.h"
#include <sstream>
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
vector<string> split(const string &s, char c = ' ',
                     size_t max_split = SIZE_MAX) {
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
        if (it < s.end() && ret.size() + 1 == max_split) {
            ret.push_back(string(it, s.end()));
            break;
        }
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
        if (n == s.end() || n + 1 == s.end()) {
            ret.push_back(string(it, s.end()));
            break;
        } else {
            if (*(n + 1) == c2) {
                if (it < n) {
                    ret.push_back(string(it, n));
                }
                it = n + 2;
            }
        }
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
    shared_ptr<HTTPRequest> r = any_cast<HTTPData &>(conn->data).r;
    vector<string> lines = split2(s);
    vector<string> l1 = split(lines[0]);
    if (l1.size() != 3) {
        http_error(conn, 413);
        return -1;
    }
    r->method = l1[0];
    r->uri = l1[1];
    r->protocol = l1[2];
    vector<string> u2 = split(r->uri, '?', 2);
    r->path = u2[0];
    if (r->path.size() == 0 || r->path[0] != '/') {
        http_error(conn, 400);
    }
    if (u2.size() >= 2) {
        r->query_string = u2[1];
    } else {
        r->query_string = "";
    }
    if (r->protocol != "HTTP/1.0" && r->protocol != "HTTP/1.1") {
        http_error(conn, 505);
        return -1;
    }
    for (size_t i = 1; i < lines.size(); i++) {
        vector<string> kv = split(lines[i], ':', 2);
        if (kv[0].size() == 0 || kv[1].size() == 0) {
            http_error(conn, 400);
        }
        r->kvs[kv[0]] = kv[1];
    }
    return 0;
}
size_t parse_fcgi_response(const string &s, string &out) {
    ostringstream ss;
    size_t a = s.find("\r\n\r\n");
    size_t b = 0;
    if (a == string::npos) {
        a = s.find("\r\n\n");
        if (a == string::npos) {
            return -1;
        } else {
            b = a + 3;
        }
    } else {
        b = a + 4;
    }
    string header = string(s.begin(), s.begin() + a);
    vector<string> lines = split2(header);
    vector<pair<string, string>> kvs;
    string status = "200 OK";
    for (string &line : lines) {
        vector<string> kv = split(line, ':', 2);
        if (kv[0] == "Status") {
            status = kv[1];
        } else {
            kvs.push_back(make_pair(kv[0], kv[1]));
        }
    }
    size_t content_length = s.size() - b;
    ss << "HTTP/1.1 " << status << "\r\n";
    ss << "Server: Httpd (Ubuntu)\r\n";
    ss << "Date: " << get_time_fmt_gmt() << "\r\n";
    ss << "Connection: keep-alive\r\n";
    ss << "Content-Length: " << content_length << "\r\n";
    for (auto &kv : kvs) {
        ss << kv.first << ": " << kv.second << "\r\n";
    }
    ss << "\r\n";
    ss << s.substr(b);
    out = ss.str();
    return content_length;
}