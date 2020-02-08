#include "http/http.h"
#include "http/error.h"
#include "http/fastcgi.h"
#include "http/parser.h"
#include "net/logging/loggingstream.h"
#include "net/util/error.h"
#include "net/util/std.h"
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
HTTP::HTTP(HTTPDConfig &_config) : config(_config) {
    logger = make_shared<Logger>(config.log);
    for (Route &route : config.routes) {
        if (route.operation == "fastcgi") {
            fcgi = make_shared<FastCGI>(route.host, route.port, config.root);
        }
    }
}
void HTTP::onConnection(shared_ptr<Connection> conn) {
    conn->data = HTTPData();
    conn->decode = bind(http_decoder, _1, _2, 65536, 0);
}
void HTTP::onMessage(shared_ptr<Connection> conn, string &input_message) {
    while (input_message.size() > 0) {
        HTTPData &d = any_cast<HTTPData &>(conn->data);
        if (d.parse_status == 0) {
            d.r = make_shared<HTTPRequest>();
            int ret = parse_header(conn, input_message);
            if (ret < 0) {
                return;
            }
            if (d.r->path == "/") {
                d.r->path += config.index;
            }
            if (d.r->kvs.find("Content-Length") != d.r->kvs.end()) {
                int content_length = stoi(d.r->kvs["Content-Length"]);
                if (content_length > 10000000) {
                    http_error(conn, 413);
                    return;
                }
                conn->decode =
                    bind(http_decoder, _1, _2, INT_MAX, content_length);
                d.parse_status = 1;
            } else {
                process_request(conn);
            }
        } else {
            d.r->content = input_message;
            d.parse_status = 0;
            process_request(conn);
            conn->decode = bind(http_decoder, _1, _2, 65536, 0);
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
void HTTP::process_request(shared_ptr<Connection> conn) {
    HTTPData &d = any_cast<HTTPData &>(conn->data);
    for (Route &route : config.routes) {
        if (match_route(route, d.r->path)) {
            if (route.operation == "fastcgi") {
                fcgi->process_request(conn, d.r);
            } else if (route.operation == "file") {
                process_file_request(conn, d.r);
            } else {
                http_error(conn, 404);
            }
            break;
        }
    }
}
void HTTP::process_file_request(shared_ptr<Connection> conn,
                                shared_ptr<HTTPRequest> r) {
    string file_path = config.root + r->path;
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
       << "Content-Length: " << size << "\r\n"
       << "Connection: Keep-Alive\r\n"
       << "Keep-Alive: timeout=5, max=1000\r\n\n";
    conn->send(os.str());
    conn->sendfile(fd);
    LOG_INFO << r->method << " " << r->uri << " " << r->protocol << " "
             << "200"
             << " " << size << "\n";
}
