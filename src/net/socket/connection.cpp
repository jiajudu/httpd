#include "net/socket/connection.h"
#include "net/schedule/connectionPool.h"
#include "net/schedule/multiplexer.h"
#include "net/schedule/timerPool.h"
#include "net/util/error.h"
#include "net/util/tm.h"
#include <sys/sendfile.h>
#include <unistd.h>
Connection::Connection(shared_ptr<Socket> _socket)
    : socket(_socket), to_close(false), closed(false) {
    buf_recv = make_shared<Buffer>();
    buf_send = make_shared<Buffer>();
}
size_t Connection::send(const string &s) {
    bool has_content = buf_send->size() > 0;
    buf_send->write(s.c_str(), s.size());
    if (pool && !has_content && buf_send->size() > 0) {
        pool->onSendBegin(shared_from_this());
    }
    return s.size();
}
void Connection::sendfile(int fd) {
    sendfile_fd = fd;
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
    if (has_content) {
        buf_send->read([this](char *s, size_t n) -> size_t {
            return socket->send(s, n, Socket::message_dont_wait);
        });
        if (pool && has_content && buf_send->size() == 0 && sendfile_fd < 0) {
            pool->onSendComplete(shared_from_this());
        }
    } else if (sendfile_fd >= 0) {
        ssize_t ret = ::sendfile(socket->get_fd(), sendfile_fd, NULL, 16384);
        while (ret >= 0) {
            if (ret == 0) {
                ::close(sendfile_fd);
                sendfile_fd = -1;
                if (pool) {
                    pool->onSendComplete(shared_from_this());
                }
                break;
            }
            ret = ::sendfile(socket->get_fd(), sendfile_fd, NULL, 16384);
        }
        if (ret < 0 && ret != EAGAIN) {
            syscall_error();
        }
    }
    if (!(has_content_to_send()) && to_close) {
        closed = true;
        if (pool) {
            pool->onClose(shared_from_this());
        }
        socket->close();
    }
    if (pool && pool->multiplexer->timers && deactivation_seconds) {
        pool->multiplexer->timers->set_deactivation(shared_from_this(),
                                                    deactivation_seconds);
    }
}
void Connection::non_blocking_recv() {
    char buf[4096];
    size_t recved = 4096;
    bool first = true;
    while (recved >= 4096) {
        recved = socket->recv(buf, 4096, Socket::message_dont_wait);
        if (first) {
            if (recved == 0) {
                close();
            }
            first = false;
        }
        buf_recv->write(buf, recved);
    }
    if (pool && pool->multiplexer->timers && deactivation_seconds) {
        pool->multiplexer->timers->set_deactivation(shared_from_this(),
                                                    deactivation_seconds);
    }
}
int Connection::close() {
    if (has_content_to_send()) {
        to_close = true;
        return 1;
    } else {
        closed = true;
        if (pool) {
            pool->onClose(shared_from_this());
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
    return buf_send->size() > 0 || sendfile_fd >= 0;
}
bool Connection::active() const {
    return !to_close && !closed;
}
void Connection::set_deactivation(int seconds) {
    deactivation_seconds = seconds;
    if (pool && pool->multiplexer->timers && deactivation_seconds) {
        pool->multiplexer->timers->set_deactivation(shared_from_this(),
                                                    deactivation_seconds);
    }
}