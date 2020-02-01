#include "socket/socket.h"
#include "auxiliary/error.h"
#include "auxiliary/ip.h"
#include <arpa/inet.h>
#include <memory>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
const int Socket::domain_INET = AF_INET;
const int Socket::domain_INET6 = AF_INET6;
const int Socket::domain_local = AF_LOCAL;
const int Socket::message_dont_wait = MSG_DONTWAIT;
Socket::Socket(int domain_, bool is_non_blocking_)
    : domain(domain_), is_non_blocking(is_non_blocking_) {
    fd = socket(domain, SOCK_STREAM, 0);
    if (fd < 0) {
        syscall_error();
    }
}
Socket::Socket(int fd_, int domain_, bool is_non_blocking_)
    : fd(fd_), domain(domain_), is_non_blocking(is_non_blocking_) {
}
int Socket::bind(string &ip, uint16_t port) {
    int ret = 0;
    if (domain == domain_INET) {
        struct sockaddr_in sockaddr;
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sin_family = domain_INET;
        sockaddr.sin_addr.s_addr = inet_ston(ip);
        sockaddr.sin_port = htons(port);
        ret = ::bind(fd, reinterpret_cast<struct sockaddr *>(&sockaddr),
                     sizeof(sockaddr));
        if (ret < 0) {
            syscall_error();
        }
    } else {
        agreement_error("protocal not implemented.");
    }
    return ret;
}
int Socket::listen(int backlog) {
    int ret = ::listen(fd, backlog);
    if (ret < 0) {
        syscall_error();
    }
    return ret;
}
int Socket::accept() {
    if (domain == domain_INET) {
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        int connfd =
            ::accept(fd, reinterpret_cast<struct sockaddr *>(&addr), &addrlen);
        if (connfd < 0) {
            syscall_error();
        }
        return connfd;
    } else {
        agreement_error("protocal not implemented.");
    }
}
ssize_t Socket::recv(char *buf, size_t size, int flag) {
    ssize_t ret = ::recv(fd, buf, size, flag);
    if (ret < 0) {
        syscall_error();
    }
    return ret;
}
ssize_t Socket::send(const char *buf, size_t size, int flag) {
    ssize_t ret = ::send(fd, buf, size, flag);
    if (ret < 0 && (errno != EPIPE && errno != ECONNRESET)) {
        syscall_error();
    }
    return ret;
}
int Socket::close() {
    int ret = ::close(fd);
    if (ret < 0) {
        syscall_error();
    }
    return ret;
}
int Socket::get_domain() const {
    return domain;
}
int Socket::get_fd() const {
    return fd;
}