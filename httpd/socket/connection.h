#pragma once
#include "auxiliary/buffer.h"
#include "auxiliary/std.h"
#include "socket/socket.h"
#include <memory>
#include <vector>
class Connection : public enable_shared_from_this<Connection> {
public:
    Connection(shared_ptr<Socket> socket, bool is_non_blocking = false);

    size_t send(const string &s);
    size_t recv(string &s,
                function<size_t(char *s_buf, size_t n_buf)> decode = 0);
    void non_blocking_send();
    void non_blocking_recv();
    int close();
    int shutdown();
    shared_ptr<Socket> get_socket() const;
    bool get_is_non_blocking() const;
    bool can_be_sent() const;
    int get_fd() const;
    bool has_content_to_send() const;
    bool active() const;
    function<void(shared_ptr<Connection>)> onClose = 0;
    function<void(shared_ptr<Connection>)> onSendBegin = 0;
    function<void(shared_ptr<Connection>)> onSendEnd = 0;

private:
    shared_ptr<Socket> socket;
    bool is_non_blocking;
    shared_ptr<Buffer> buf_recv;
    shared_ptr<Buffer> buf_send;
    bool to_close;
    bool closed;
};