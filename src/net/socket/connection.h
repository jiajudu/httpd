#pragma once
#include "net/socket/socket.h"
#include "net/util/buffer.h"
#include "net/util/std.h"
#include <any>
#include <limits.h>
#include <memory>
#include <time.h>
#include <vector>
class ConnectionPool;
class Connection : public enable_shared_from_this<Connection> {
public:
    Connection(shared_ptr<Socket> socket);
    size_t send(const string &s);
    void sendfile(int fd);
    size_t recv(string &s);
    void non_blocking_send();
    void non_blocking_recv();
    int close();
    int shutdown();
    shared_ptr<Socket> get_socket() const;
    bool can_be_sent() const;
    int get_fd() const;
    bool has_content_to_send() const;
    bool active() const;
    function<size_t(char *s_buf, size_t n_buf)> decode = 0;
    any data;
    time_t deactivation_time = LONG_MAX;
    void set_deactivation(int seconds);
    shared_ptr<ConnectionPool> pool;

private:
    shared_ptr<Socket> socket;
    shared_ptr<Buffer> buf_recv;
    shared_ptr<Buffer> buf_send;
    bool to_close;
    bool closed;
    int deactivation_seconds = 0;
    int sendfile_fd = -1;
};