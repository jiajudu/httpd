#include "service/http.h"
#include "auxiliary/error.h"
#include "fastcgi/fastcgi.h"
#include <string.h>
#include <unistd.h>
size_t http_decoder(char *s_buf, size_t n_buf, size_t max_len,
                    size_t expected_length) {
    if (expected_length == 0) {
        char *start = s_buf;
        char *end = s_buf + n_buf;
        long ret = 0;
        char *p = find(start, end - 3, '\r');
        while (p < end - 4) {
            if (*(p + 1) == '\n' && *(p + 2) == '\r' && *(p + 3) == '\n') {
                ret = p + 3 - s_buf;
                break;
            } else {
                start = p + 1;
            }
            if (start >= end - 3) {
                break;
            }
            if (static_cast<size_t>(start - s_buf) >= max_len) {
                return max_len;
            }
            p = find(start, end - 3, '\r');
            return ret;
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
HTTP::HTTP() {
    char buf[256];
    char *ret = getcwd(buf, 256);
    if (ret == 0) {
        fatal_error("Cannot get current working directory");
    }
    size_t len = strlen(buf);
    cwd = string(buf, buf + len);
    set_header2env();
}
void HTTP::set_header2env() {
    header2env["Host"] = "HTTP_HOST";
    header2env["Connection"] = "HTTP_CONNECTION";
    header2env["Cache-Control"] = "HTTP_CACHE_CONTROL";
    header2env["Upgrade-Insecure-Requests"] = "HTTP_UPGRADE_INSECURE_REQUESTS";
    header2env["User-Agent"] = "HTTP_USER_AGENT";
    header2env["Sec-Fetch-User"] = "HTTP_SEC_FETCH_USER";
    header2env["Accept"] = "HTTP_ACCEPT";
    header2env["Sec-Fetch-Site"] = "HTTP_SEC_FETCH_SITE";
    header2env["Sec-Fetch-Mode"] = "HTTP_SEC_FETCH_MODE";
    header2env["Accept-Encoding"] = "HTTP_ACCEPT_ENCODING";
    header2env["Accept-Language"] = "HTTP_ACCEPT_LANGUAGE";
    header2env["Cookie"] = "HTTP_COOKIE";
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
                    error(conn, 413);
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
                error(conn, 400);
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
        error(conn, 413);
        return -1;
    }
    string::iterator it1 = find(s.begin(), s.end(), ' ');
    string::iterator it2 = find(it1 + 1, s.end(), ' ');
    string method(s.begin(), it);
    string::iterator it3 = find(it1 + 1, it2, '?');
    string path(it1 + 1, it3);
    if (path.size() == 0 || path[0] != '/') {
        error(conn, 400);
    }
    string query_string;
    if (it3 + 1 < it2) {
        query_string = string(it3 + 1, it2);
    }
    string protocol(it2 + 1, s.end());
    if (protocol != "HTTP/1.0" && protocol != "HTTP/1.1") {
        error(conn, 505);
        return -1;
    }
    HTTPRequest &r = any_cast<HTTPRequest &>(conn->data);
    r.method = method;
    r.uri = s;
    r.path = path;
    r.query_string = query_string;
    r.protocol = protocol;
    for (size_t i = 1; i < lines.size(); i++) {
        it = find(lines[i].begin(), lines[i].end(), ':');
        if (it == lines[i].end()) {
            error(conn, 400);
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
            error(conn, 400);
        }
        r.kvs[key] = value;
    }
    return 0;
}
void HTTP::error(shared_ptr<Connection> conn, int code) {
    (void)conn;
    (void)code;
}
void HTTP::process_request(shared_ptr<Connection> conn) {
    FastCGITask task;
    HTTPRequest &r = any_cast<HTTPRequest &>(conn->data);
    for (auto &kv : r.kvs) {
        if (header2env.find(kv.first) != header2env.end()) {
            task.add_env(kv.first, kv.second);
        } else {
            fatal_error((string("Unexpected header ") + kv.first).c_str());
            error(conn, 400);
        }
    }
    task.add_env(REDIRECT_STATUS, "200");
    task.add_env(SERVER_NAME, "_");
    task.add_env(SERVER_PORT, to_string(conn->get_socket()->get_local_port()));
    task.add_env(SERVER_ADDR, conn->get_socket()->get_local_ip());
    task.add_env(REMOTE_PORT, to_string(conn->get_socket()->get_remote_port()));
    task.add_env(REMOTE_ADDR, conn->get_socket()->get_remote_ip());
    task.add_env(SERVER_SOFTWARE, "httpd/0.0.1");
    task.add_env(GATEWAY_INTERFACE, "FastCGI/1.1");
    task.add_env(REQUEST_SCHEME, "http");
    task.add_env(DOCUMENT_ROOT, cwd);
    task.add_env(DOCUMENT_URI, r.path);
    task.add_env(REQUEST_URI, r.uri);
    task.add_env(SCRIPT_NAME, r.path);
    if (r.kvs.find("Content-Length") != r.kvs.end()) {
        task.add_env(CONTENT_LENGTH, r.kvs["Content-Length"]);
    } else {
        task.add_env(CONTENT_LENGTH, "0");
    }
    task.add_env(CONTENT_TYPE, "");
    task.add_env(REQUEST_METHOD, r.method);
    task.add_env(QUERY_STRING, r.query_string);
    task.add_env(SERVER_PROTOCOL, r.protocol);
    task.add_env(SCRIPT_FILENAME, cwd + r.path);
    task.add_env(FCGI_ROLE, "RESPONDER");
    size_t ptr = 0;
    while (ptr < r.content.size()) {
        if (ptr + 32768 <= r.content.size()) {
            task.add_content(string(&r.content[ptr], &r.content[32768]));
            ptr += 32768;
        } else {
            task.add_content(string(
                &r.content[ptr], &r.content[ptr] + (r.content.size() - ptr)));
            ptr = r.content.size();
        }
    }
    shared_ptr<FastCGI> fcgi = any_cast<shared_ptr<FastCGI>>(tl());
    fcgi->submit(task, conn);
}