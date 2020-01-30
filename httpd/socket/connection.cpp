#include "socket/connection.h"
#include "auxiliary/error.h"
Connection::Connection(shared_ptr<Socket> _socket, bool _is_non_blocking)
    : socket(_socket), is_non_blocking(_is_non_blocking), to_close(false),
      closed(false) {
    if (is_non_blocking) {
        buf_recv = make_shared<Buffer>();
        buf_send = make_shared<Buffer>();
    }
}
size_t Connection::send(const string &s) {
    if (is_non_blocking) {
        buf_send->write(s.c_str(), s.size());
        return s.size();
    } else {
        return socket->send(s.c_str(), s.size(), 0);
    }
}
size_t Connection::recv(string &s,
                        function<size_t(char *s_buf, size_t n_buf)> decode) {
    if (is_non_blocking) {
        if (decode == 0) {
            agreement_error("decoder cannot be null");
        }
        return buf_recv->read([&](char *s_buf, size_t n_buf) -> size_t {
            size_t ret = decode(s_buf, n_buf);
            string str(s_buf, s_buf + ret);
            swap(s, str);
            return ret;
        });
    } else {
        return socket->recv(const_cast<char *>(s.c_str()), s.size(), 0);
    }
}
void Connection::non_blocking_send() {
    if (is_non_blocking) {
        buf_send->read([this](char *s, size_t n) -> size_t {
            return socket->send(s, n, Socket::message_dont_wait);
        });
        if (buf_send->size() == 0 && to_close) {
            closed = true;
            socket->close();
        }
    }
}
void Connection::non_blocking_recv() {
    if (is_non_blocking) {
        char buf[4096];
        size_t recved = 4096;
        while (recved >= 4096) {
            recved = socket->recv(buf, 4096, Socket::message_dont_wait);
            buf_recv->write(buf, recved);
        }
    }
}
int Connection::close() {
    if (is_non_blocking) {
        if (buf_send->size() == 0) {
            closed = true;
            return socket->close();
        } else {
            to_close = true;
            return 1;
        }
    } else {
        closed = true;
        return socket->close();
    }
}
shared_ptr<Socket> Connection::get_socket() const {
    return socket;
}
bool Connection::get_is_non_blocking() const {
    return is_non_blocking;
}
bool Connection::can_be_sent() const {
    return !buf_recv && !buf_send && !to_close && !closed;
}
int Connection::get_fd() const {
    return socket->get_fd();
}
bool Connection::has_content_to_send() const {
    return is_non_blocking && buf_send->size() > 0;
}