#include "fastcgi/fastcgi.h"
#include "auxiliary/error.h"
#include "schedule/connectorPool.h"
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
FastCGI::FastCGI(shared_ptr<Multiplexer> _m, string &ip, uint16_t port)
    : m(_m), fcgi_ip(ip), fcgi_port(port) {
}
void FastCGI::submit(FastCGITask task, shared_ptr<Connection> _c) {
    uint16_t id = counter;
    counter++;
    if (tasks.find(id) != tasks.end()) {
        fatal_error("The request id has been used.");
    }
    m->connectors->connect(
        fcgi_ip, fcgi_port,
        bind(&FastCGI::onConnectionEstablished, this, _1, id),
        bind(&FastCGI::onConnectionError, this, _1, id));
    tasks[id] = task;
    tasks[id].http_conn = _c;
}
void FastCGI::onConnectionEstablished(shared_ptr<Connection> _conn,
                                      uint16_t id) {
    _conn->decode = bind(&FastCGI::decode, this, _1, _2);
    tasks[id].fcgi_conn = _conn;
    shared_ptr<ConnectionEvent> ev = make_shared<ConnectionEvent>();
    ev->onMessage = bind(&FastCGI::onMessage, this, _1, _2, id);
    m->connections->add_connection(_conn, ev);
    sendBeginRequest(_conn, id);
    sendParams(_conn, id, tasks[id].envs);
    sendStdins(_conn, id, tasks[id].stdins);
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
        if (tasks.find(request_id) == tasks.end()) {
            fatal_error("The request id is invalid.");
        }
        if (message_type == FCGI_STDOUT) {
            tasks[request_id].stdin += message.substr(sizeof(FCGI_Header), len);
        } else if (message_type == FCGI_STDERR) {
            tasks[request_id].stderr +=
                message.substr(sizeof(FCGI_Header), len);
        } else if (message_type == FCGI_END_REQUEST) {
            tasks[request_id].http_conn->send(tasks[request_id].stdin);
            tasks[request_id].http_conn->close();
            tasks[request_id].fcgi_conn->close();
            tasks.erase(request_id);
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