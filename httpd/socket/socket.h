#pragma once
#include "auxiliary/noncopyable.h"
#include "auxiliary/std.h"
#include <memory>
#include <string>
class Socket : Noncopyable {
public:
    Socket(int domain_);
    Socket(int fd_, int domain_);
    int bind(string &ip, uint16_t port);
    int listen(int backlog);
    int accept();
    int connect(string &ip, uint16_t port);
    int close();
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