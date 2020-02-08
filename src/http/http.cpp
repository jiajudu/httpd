#include "http/http.h"
#include "http/error.h"
#include "http/fastcgi.h"
#include "net/logging/loggingstream.h"
#include "net/util/error.h"
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static size_t http_decoder(char *s_buf, size_t n_buf, size_t max_len,
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
HTTP::HTTP(HTTPDConfig &_config) : config(_config) {
    logger = make_shared<Logger>(config.log);
}
void HTTP::init(shared_ptr<Scheduler> scheduler) {
    for (Route &route : config.routes) {
        if (route.operation == "fcgi") {
            fcgi() = make_shared<FastCGI>(scheduler, route.host, route.port,
                                          config.root);
        }
    }
}
void HTTP::onConnection(shared_ptr<Connection> conn) {
    conn->data = HTTPRequest();
    conn->decode = bind(http_decoder, _1, _2, 65536, 0);
}
void HTTP::onMessage(shared_ptr<Connection> conn, string &input_message) {
    while (input_message.size() > 0) {
        HTTPRequest &r = any_cast<HTTPRequest &>(conn->data);
        if (r.parse_status == 0) {
            int ret = parse_header(conn, input_message);
            if (ret < 0) {
                return;
            }
            string c("Content-Length");
            if (r.kvs.find(c) != r.kvs.end()) {
                int content_length = stoi(r.kvs[c]);
                if (content_length > 10000000) {
                    http_error(conn, 413);
                    return;
                }
                conn->decode =
                    bind(http_decoder, _1, _2, INT_MAX, content_length + 4);
                r.parse_status = 1;
            } else {
                process_request(conn);
            }
        } else {
            size_t size = input_message.size();
            if (input_message[size - 1] != '\n' ||
                input_message[size - 1] != '\r' ||
                input_message[size - 1] != '\n' ||
                input_message[size - 1] != '\r') {
                http_error(conn, 400);
                return;
            }
            for (int i = 0; i < 4; i++) {
                input_message.pop_back();
            }
            r.content = input_message;
            r.parse_status = 0;
            process_request(conn);
        }
        conn->recv(input_message);
    }
}
void HTTP::onSendComplete(shared_ptr<Connection> conn) {
    (void)conn;
}
void HTTP::onDisconnect(shared_ptr<Connection> conn) {
    (void)conn;
}
int HTTP::parse_header(shared_ptr<Connection> conn, string &s) {
    HTTPRequest &r = any_cast<HTTPRequest &>(conn->data);
    vector<string> lines;
    string::iterator it = s.begin();
    while (it + 1 < s.end()) {
        string::iterator nit = find(it, s.end(), '\r');
        if (nit + 1 < s.end() && *(nit + 1) == '\n') {
            if (it < nit) {
                lines.push_back(string(it, nit));
            }
            it = nit + 2;
        }
    }
    ptrdiff_t c = count(lines[0].begin(), lines[0].end(), ' ');
    if (c != 2) {
        http_error(conn, 413);
        return -1;
    }
    string::iterator it1 = find(lines[0].begin(), lines[0].end(), ' ');
    string::iterator it2 = find(it1 + 1, lines[0].end(), ' ');
    r.method = string(lines[0].begin(), it1);
    r.uri = string(it1 + 1, it2);
    string::iterator it3 = find(it1 + 1, it2, '?');
    r.path = string(it1 + 1, it3);
    if (r.path.size() == 0 || r.path[0] != '/') {
        http_error(conn, 400);
    }
    if (it3 + 1 < it2) {
        r.query_string = string(it3 + 1, it2);
    } else {
        r.query_string = "";
    }
    r.protocol = string(it2 + 1, lines[0].end());
    if (r.protocol != "HTTP/1.0" && r.protocol != "HTTP/1.1") {
        http_error(conn, 505);
        return -1;
    }
    for (size_t i = 1; i < lines.size(); i++) {
        it = find(lines[i].begin(), lines[i].end(), ':');
        if (it == lines[i].end()) {
            http_error(conn, 400);
            return -1;
        }
        string key = string(lines[i].begin(), it);
        string value = string(it + 1, lines[i].end());
        while (key.back() == ' ') {
            key.pop_back();
        }
        while (key.front() == ' ') {
            key.erase(key.begin());
        }
        while (value.back() == ' ') {
            value.pop_back();
        }
        while (value.front() == ' ') {
            value.erase(value.begin());
        }
        if (key.size() == 0 || value.size() == 0) {
            http_error(conn, 400);
        }
        r.kvs[key] = value;
    }
    return 0;
}
void HTTP::process_request(shared_ptr<Connection> conn) {
    HTTPRequest &r = any_cast<HTTPRequest &>(conn->data);
    for (Route &route : config.routes) {
        if (match_route(route, r.path)) {
            if (route.operation == "fastcgi") {
                fcgi()->process_request(conn, r);
            } else if (route.operation == "file") {
                process_file_request(conn, r);
            } else {
                http_error(conn, 404);
            }
            break;
        }
    }
}
void HTTP::process_file_request(shared_ptr<Connection> conn, HTTPRequest &r) {
    if (r.uri == "/") {
        r.uri += config.index;
    }
    string file_path = config.root + r.uri;
    struct stat st;
    int ret = stat(file_path.c_str(), &st);
    if (ret < 0) {
        http_error(conn, 404);
        return;
    }
    off_t size = st.st_size;
    // struct timespec last_modification = st.st_mtim;
    int fd = open(file_path.c_str(), O_RDONLY);
    ostringstream os;
    os << "HTTP/1.1 200 OK\r\n"
       << "Content-Length: " << size << "\r\n\n";
    conn->send(os.str());
    conn->sendfile(fd);
    LOG_INFO << r.method << " " << r.uri << " " << r.protocol << " "
             << "200"
             << " " << size << "\n";
}
shared_ptr<FastCGI> &HTTP::fcgi() {
    thread_local shared_ptr<FastCGI> p;
    return p;
}