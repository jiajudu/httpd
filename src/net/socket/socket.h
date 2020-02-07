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
    int accept();
    int connect(string &ip, uint16_t port, bool non_blocking);
    int close();
    ssize_t recv(char *buf, size_t size, int flag);
    ssize_t send(const char *buf, size_t size, int flag);
    static const int domain_INET;
    static const int domain_INET6;
    static const int domain_local;
    static const int message_dont_wait;
    int get_domain() const;
    int get_fd() const;
    string get_local_ip() const;
    uint16_t get_local_port() const;
    string get_remote_ip() const;
    uint16_t get_remote_port() const;

private:
    int fd;
    int domain;
    string local_ip;
    uint16_t local_port;
    string remote_ip;
    uint16_t remote_port;
};