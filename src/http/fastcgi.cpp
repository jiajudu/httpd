#include "http/fastcgi.h"
#include "http/error.h"
#include "http/parser.h"
#include "net/logging/loggingstream.h"
#include "net/schedule/connectorPool.h"
#include "net/util/error.h"
#include <iostream>
#include <string.h>
void FastCGITask::add_env(const string &key, const string &value) {
    size_t len = key.size() + value.size() + 2;
    if (key.size() >= 128) {
        len += 3;
    }
    if (value.size() >= 128) {
        len += 3;
    }
    string r(len, 0);
    int ptr = 0;
    if (key.size() < 128) {
        size_t size = key.size();
        r[ptr] = static_cast<char>(size);
        ptr++;
    } else {
        size_t size = key.size();
        r[ptr] = static_cast<char>((size >> 24) | 0x80);
        r[ptr + 1] = static_cast<char>((size >> 16) & 0xff);
        r[ptr + 2] = static_cast<char>((size >> 8) & 0xff);
        r[ptr + 3] = static_cast<char>(size & 0xff);
        ptr += 4;
    }
    if (value.size() < 128) {
        size_t size = value.size();
        r[ptr] = static_cast<char>(size);
        ptr++;
    } else {
        size_t size = value.size();
        r[ptr] = static_cast<char>((size >> 24) | 0x80);
        r[ptr + 1] = static_cast<char>((size >> 16) & 0xff);
        r[ptr + 2] = static_cast<char>((size >> 8) & 0xff);
        r[ptr + 3] = static_cast<char>(size & 0xff);
        ptr += 4;
    }
    string::iterator it = r.begin() + ptr;
    it = copy(key.begin(), key.end(), it);
    it = copy(value.begin(), value.end(), it);
    if (envs.size() == 0 || envs.back().size() + r.size() > 32768) {
        envs.push_back(r);
    } else {
        envs.back() += r;
    }
}
void FastCGITask::add_content(const string &c) {
    stdins.push_back(c);
}
FastCGI::FastCGI(const string &_fcgi_host, uint16_t _fcgi_port,
                 const string &_root, shared_ptr<Logger> _logger)
    : fcgi_host(_fcgi_host), fcgi_port(_fcgi_port), root(_root),
      logger(_logger) {
    set_header2env();
}
void FastCGI::set_header2env() {
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
    header2env["Referer"] = "HTTP_REFERER";
    header2env["Content-Length"] = "HTTP_CONTENT_LENGTH";
    header2env["Origin"] = "HTTP_ORIGIN";
    header2env["Content-Type"] = "HTTP_CONTENT_TYPE";
}
void FastCGI::process_request(shared_ptr<Connection> conn,
                              shared_ptr<HTTPRequest> r) {
    shared_ptr<FastCGITask> task = make_shared<FastCGITask>();
    for (auto &kv : r->kvs) {
        if (header2env.find(kv.first) != header2env.end()) {
            task->add_env(header2env[kv.first], kv.second);
        }
    }
    task->add_env("REDIRECT_STATUS", "200");
    task->add_env("SERVER_NAME", "_");
    task->add_env("SERVER_PORT", to_string(conn->local_port));
    task->add_env("SERVER_ADDR", conn->local_ip);
    task->add_env("REMOTE_PORT", to_string(conn->remote_port));
    task->add_env("REMOTE_ADDR", conn->remote_ip);
    task->add_env("SERVER_SOFTWARE", "httpd/0.0.1");
    task->add_env("GATEWAY_INTERFACE", "CGI/1.1");
    task->add_env("REQUEST_SCHEME", "http");
    task->add_env("DOCUMENT_ROOT", root);
    task->add_env("DOCUMENT_URI", r->path);
    task->add_env("REQUEST_URI", r->uri);
    task->add_env("SCRIPT_NAME", r->path);
    if (r->kvs.find("Content-Length") != r->kvs.end()) {
        task->add_env("CONTENT_LENGTH", r->kvs["Content-Length"]);
    } else {
        task->add_env("CONTENT_LENGTH", "");
    }
    task->add_env("CONTENT_TYPE", r->kvs["Content-Type"]);
    task->add_env("REQUEST_METHOD", r->method);
    task->add_env("QUERY_STRING", r->query_string);
    task->add_env("SERVER_PROTOCOL", r->protocol);
    task->add_env("SCRIPT_FILENAME", root + r->path);
    size_t ptr = 0;
    while (ptr < r->content.size()) {
        if (ptr + 32768 <= r->content.size()) {
            task->add_content(string(&r->content[ptr], &r->content[32768]));
            ptr += 32768;
        } else {
            task->add_content(
                string(&r->content[ptr],
                       &r->content[ptr] + (r->content.size() - ptr)));
            ptr = r->content.size();
        }
    }
    uint16_t id = 0;
    {
        lock_guard<mutex> g(lock);
        id = 1;
        while (tasks.find(id) != tasks.end()) {
            id++;
        }
        tasks[id] = task;
        tasks[id]->http_conn = conn;
        tasks[id]->request = r;
        tasks[id]->scheduler = conn->pool->scheduler;
    }
    conn->pool->scheduler->connectors->connect(
        fcgi_host, fcgi_port,
        bind(&FastCGI::onConnectionEstablished, this, _1, id),
        bind(&FastCGI::onConnectionError, this, _1, id));
}
void FastCGI::onConnectionEstablished(shared_ptr<Connection> _conn,
                                      uint16_t id) {
    _conn->decode = bind(&FastCGI::decode, this, _1, _2);
    shared_ptr<FastCGITask> task;
    {
        lock_guard<mutex> g(lock);
        task = tasks[id];
    }
    task->fcgi_conn = _conn;
    shared_ptr<ConnectionEvent> ev = make_shared<ConnectionEvent>();
    ev->onMessage = bind(&FastCGI::onMessage, this, _1, _2, id);
    task->scheduler->connections->add_connection(_conn, ev);
    sendBeginRequest(_conn, id);
    sendParams(_conn, id, task->envs);
    sendStdins(_conn, id, task->stdins);
}
void FastCGI::onConnectionError(shared_ptr<Connection> _conn, uint16_t id) {
    (void)_conn;
    (void)id;
    fatal_error("Connect connect to the FastCGI Server.");
}
void FastCGI::onMessage(shared_ptr<Connection> _conn, string &message,
                        uint16_t id) {
    while (message.size() > 0) {
        unsigned char message_type = static_cast<unsigned char>(message[1]);
        uint16_t request_id = static_cast<uint16_t>(
            ((static_cast<unsigned char>(message[2]) << 8) +
             static_cast<unsigned char>(message[3])));
        size_t len =
            (static_cast<size_t>(static_cast<unsigned char>(message[4])) << 8) +
            static_cast<unsigned char>(message[5]);
        if (id != request_id) {
            fatal_error("Request id mismatch.");
        }
        shared_ptr<FastCGITask> task;
        {
            lock_guard<mutex> g(lock);
            if (tasks.find(request_id) == tasks.end()) {
                fatal_error("The request id is invalid.");
            }
            task = tasks[id];
        }
        if (message_type == FCGI_STDOUT) {
            task->stdin += message.substr(sizeof(FCGI_Header), len);
        } else if (message_type == FCGI_STDERR) {
            task->stderr += message.substr(sizeof(FCGI_Header), len);
        } else if (message_type == FCGI_END_REQUEST) {
            string response;
            size_t content_length = parse_fcgi_response(task->stdin, response);
            task->http_conn->send(response);
            LOG_INFO << task->request->method << " " << task->request->uri
                     << " " << task->request->protocol << " "
                     << "200"
                     << " " << content_length << "\n";
            if (task->stderr.size() > 0) {
                LOG_WARN << task->stderr << "\n";
            }
            task->http_conn->close();
            task->fcgi_conn->close();
            {
                lock_guard<mutex> g(lock);
                tasks.erase(request_id);
            }
        }
        _conn->recv(message);
    }
}
size_t FastCGI::decode(char *s_buf, size_t n_buf) {
    if (n_buf < sizeof(FCGI_Header)) {
        return 0;
    } else {
        size_t len =
            (static_cast<size_t>(static_cast<unsigned char>(s_buf[4])) << 8) +
            static_cast<unsigned char>(s_buf[5]) +
            static_cast<unsigned char>(s_buf[6]);
        if (n_buf < sizeof(FCGI_Header) + len) {
            return 0;
        } else {
            return sizeof(FCGI_Header) + len;
        }
    }
}
void FastCGI::sendBeginRequest(shared_ptr<Connection> _conn, uint16_t id) {
    FCGI_Header header;
    header.version = 1;
    header.type = FCGI_BEGIN_REQUEST;
    header.requestIdB1 = static_cast<unsigned char>(id >> 8);
    header.requestIdB0 = static_cast<unsigned char>(id & 0xff);
    header.contentLengthB1 = sizeof(FCGI_BeginRequestBody) >> 8;
    header.contentLengthB0 = sizeof(FCGI_BeginRequestBody) & 0xff;
    header.paddingLength = 0;
    header.reserved = 0;
    FCGI_BeginRequestBody b;
    b.roleB1 = FCGI_RESPONDER >> 8;
    b.roleB0 = FCGI_RESPONDER & 0xff;
    b.flags = 1;
    memset(b.reserved, 0, 5);
    string h(sizeof(FCGI_Header) + sizeof(FCGI_BeginRequestBody), 0);
    memcpy(&h[0], &header, sizeof(FCGI_Header));
    memcpy(&h[0] + sizeof(FCGI_Header), &b, sizeof(FCGI_BeginRequestBody));
    _conn->send(h);
}
void FastCGI::sendParams(shared_ptr<Connection> _conn, uint16_t id,
                         vector<string> &ps) {
    FCGI_Header header;
    header.version = 1;
    header.type = FCGI_PARAMS;
    header.requestIdB1 = static_cast<unsigned char>(id >> 8);
    header.requestIdB0 = static_cast<unsigned char>(id & 0xff);
    header.paddingLength = 0;
    header.reserved = 0;
    string h(sizeof(FCGI_Header), 0);
    for (string &s : ps) {
        header.contentLengthB1 = static_cast<unsigned char>(s.size() >> 8);
        header.contentLengthB0 = static_cast<unsigned char>(s.size() & 0xff);
        memcpy(&h[0], &header, sizeof(FCGI_Header));
        _conn->send(h);
        _conn->send(s);
    }
    header.contentLengthB1 = static_cast<unsigned char>(0);
    header.contentLengthB0 = static_cast<unsigned char>(0);
    memcpy(&h[0], &header, sizeof(FCGI_Header));
    _conn->send(h);
}
void FastCGI::sendStdins(shared_ptr<Connection> _conn, uint16_t id,
                         vector<string> &ps) {
    FCGI_Header header;
    header.version = 1;
    header.type = FCGI_STDIN;
    header.requestIdB1 = static_cast<unsigned char>(id >> 8);
    header.requestIdB0 = static_cast<unsigned char>(id & 0xff);
    header.paddingLength = 0;
    header.reserved = 0;
    string h(sizeof(FCGI_Header), 0);
    for (string &s : ps) {
        header.contentLengthB1 = static_cast<unsigned char>(s.size() >> 8);
        header.contentLengthB0 = static_cast<unsigned char>(s.size() & 0xff);
        memcpy(&h[0], &header, sizeof(FCGI_Header));
        _conn->send(h);
        _conn->send(s);
    }
    header.contentLengthB1 = static_cast<unsigned char>(0);
    header.contentLengthB0 = static_cast<unsigned char>(0);
    memcpy(&h[0], &header, sizeof(FCGI_Header));
    _conn->send(h);
}