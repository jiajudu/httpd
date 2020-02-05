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
vector<string> &FastCGITask::get_envs() {
    return envs;
}
vector<string> &FastCGITask::get_stdins() {
    return stdins;
}
FastCGI::FastCGI(string &ip, uint16_t address, shared_ptr<Multiplexer> _m)
    : m(_m) {
    m->connectors->connect(ip, address,
                           bind(&FastCGI::onConnectionEstablished, this, _1),
                           bind(&FastCGI::onConnectionError, this, _1));
}
bool FastCGI::available() const {
    return is_available;
}
void FastCGI::onConnectionEstablished(shared_ptr<Connection> _conn) {
    conn = _conn;
    shared_ptr<ConnectionEvent> ev = make_shared<ConnectionEvent>();
    ev->onMessage = bind(&FastCGI::onMessage, this, _1, _2);
    m->connections->add_connection(_conn, ev);
    is_available = true;
}
void FastCGI::onConnectionError(shared_ptr<Connection> _conn) {
    (void)_conn;
    fatal_error("Connect to the FastCGI Server");
}
void FastCGI::onMessage(shared_ptr<Connection> _conn, string &message) {
    while (message.size() > 0) {
        unsigned char message_type = static_cast<unsigned char>(message[1]);
        uint16_t request_id = static_cast<uint16_t>(
            ((static_cast<unsigned char>(message[2]) << 8) +
             static_cast<unsigned char>(message[3])));
        if (tasks.find(request_id) == tasks.end()) {
            fatal_error("The request id is invalid.");
        }
        if (message_type == FCGI_STDOUT) {
            tasks[request_id].stdin += message.substr(sizeof(FCGI_Header));
        } else if (message_type == FCGI_STDERR) {
            tasks[request_id].stderr += message.substr(sizeof(FCGI_Header));
        } else if (message_type == FCGI_END_REQUEST) {
            tasks[request_id].conn->send(tasks[request_id].stdin);
            tasks[request_id].conn->close();
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
            s_buf[5];
        if (n_buf < sizeof(FCGI_Header) + len) {
            return 0;
        } else {
            return sizeof(FCGI_Header) + len;
        }
    }
}
void FastCGI::submit(FastCGITask task, shared_ptr<Connection> _c) {
    uint16_t id = counter;
    if (tasks.find(id) != tasks.end()) {
        fatal_error("The request id has been used.");
    }
    counter++;
    tasks[id].conn = _c;
    sendBeginRequest(id);
    sendParams(id, task.get_envs());
    sendStdins(id, task.get_stdins());
}
void FastCGI::sendBeginRequest(uint16_t id) {
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
    conn->send(h);
}
void FastCGI::sendParams(uint16_t id, vector<string> &ps) {
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
        conn->send(h);
        conn->send(s);
    }
    header.requestIdB1 = static_cast<unsigned char>(id >> 8);
    header.requestIdB0 = static_cast<unsigned char>(id & 0xff);
    memcpy(&h[0], &header, sizeof(FCGI_Header));
    conn->send(h);
}
void FastCGI::sendStdins(uint16_t id, vector<string> &ps) {
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
        conn->send(h);
        conn->send(s);
    }
    header.requestIdB1 = static_cast<unsigned char>(id >> 8);
    header.requestIdB0 = static_cast<unsigned char>(id & 0xff);
    memcpy(&h[0], &header, sizeof(FCGI_Header));
    conn->send(h);
}