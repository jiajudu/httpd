#pragma once
#include "net/util/noncopyable.h"
#include "net/util/std.h"
#include <memory>
#include <string>
class Socket : Noncopyable {
public:
    Socket(int domain_);
    Socket(int fd_, int domain_);
    int bind(string &ip, uint16_t port);
    int listen(int backlog);
    int accept(string &remote_ip, uint16_t &remote_port);
    int connect(const string &ip, uint16_t port, bool non_blocking);
    int close();
    void reuse_addr();
    void get_name(string &local_ip, uint16_t &local_port);
    void get_peer_name(string &remote_ip, uint16_t &remote_port);
    ssize_t recv(char *buf, size_t size, int flag);
    ssize_t send(const char *buf, size_t size, int flag);
    static const int domain_INET;
    static const int domain_INET6;
    static const int domain_local;
    static const int message_dont_wait;
    int get_domain() const;
    int get_fd() const;

private:
    int fd;
    int domain;
};