#include "socket/connection.h"
#include "auxiliary/error.h"
#include "auxiliary/tm.h"
Connection::Connection(shared_ptr<Socket> _socket)
    : socket(_socket), to_close(false), closed(false) {
    buf_recv = make_shared<Buffer>();
    buf_send = make_shared<Buffer>();
}
size_t Connection::send(const string &s) {
    bool has_content = buf_send->size() > 0;
    buf_send->write(s.c_str(), s.size());
    if (onSendBegin && !has_content && buf_send->size() > 0) {
        onSendBegin(shared_from_this());
    }
    return s.size();
}
size_t Connection::recv(string &s) {
    return buf_recv->read([&](char *s_buf, size_t n_buf) -> size_t {
        size_t ret = n_buf;
        if (decode != 0) {
            ret = decode(s_buf, n_buf);
        }
        string str(s_buf, s_buf + ret);
        swap(s, str);
        return ret;
    });
}
void Connection::non_blocking_send() {
    bool has_content = buf_send->size() > 0;
    buf_send->read([this](char *s, size_t n) -> size_t {
        return socket->send(s, n, Socket::message_dont_wait);
    });
    if (onSendComplete && has_content && buf_send->size() == 0) {
        onSendComplete(shared_from_this());
    }
    if (buf_send->size() == 0 && to_close) {
        closed = true;
        if (onClose) {
            onClose(shared_from_this());
        }
        socket->close();
    }
    timer->set_deactivation(shared_from_this(), deactivation_seconds);
}
void Connection::non_blocking_recv() {
    char buf[4096];
    size_t recved = 4096;
    while (recved >= 4096) {
        recved = socket->recv(buf, 4096, Socket::message_dont_wait);
        buf_recv->write(buf, recved);
    }
    timer->set_deactivation(shared_from_this(), deactivation_seconds);
}
int Connection::close() {
    if (has_content_to_send()) {
        to_close = true;
        return 1;
    } else {
        closed = true;
        if (onClose) {
            onClose(shared_from_this());
        }
        return socket->close();
    }
}
int Connection::shutdown() {
    closed = true;
    return socket->close();
}
shared_ptr<Socket> Connection::get_socket() const {
    return socket;
}
bool Connection::can_be_sent() const {
    return (!buf_recv || buf_recv->size() == 0) &&
           (!buf_send || buf_send->size() == 0) && !to_close && !closed;
}
int Connection::get_fd() const {
    return socket->get_fd();
}
bool Connection::has_content_to_send() const {
    return buf_send->size() > 0;
}
bool Connection::active() const {
    return !to_close && !closed;
}
void Connection::set_deactivation(int seconds) {
    deactivation_seconds = seconds;
    timer->set_deactivation(shared_from_this(), deactivation_seconds);
}