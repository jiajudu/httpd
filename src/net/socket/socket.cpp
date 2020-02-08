#include "net/socket/socket.h"
#include "net/util/error.h"
#include "net/util/ip.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <memory>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
const int Socket::domain_INET = AF_INET;
const int Socket::domain_INET6 = AF_INET6;
const int Socket::domain_local = AF_LOCAL;
const int Socket::message_dont_wait = MSG_DONTWAIT;
Socket::Socket(int domain_) : domain(domain_) {
    fd = socket(domain, SOCK_STREAM, 0);
    if (fd < 0) {
        syscall_error();
    }
}
Socket::Socket(int fd_, int domain_) : fd(fd_), domain(domain_) {
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
        local_ip = ip;
        local_port = port;
    } else {
        fatal_error("Protocal not implemented.");
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
        remote_ip = inet_ntos(addr.sin_addr.s_addr);
        remote_port = ntohs(addr.sin_port);
        return connfd;
    } else {
        fatal_error("Protocal not implemented.");
    }
}
int Socket::connect(const string &ip, uint16_t port, bool non_blocking) {
    int ret = 0;
    if (domain == domain_INET) {
        struct sockaddr_in sockaddr;
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sin_family = domain_INET;
        sockaddr.sin_addr.s_addr = inet_ston(ip);
        sockaddr.sin_port = htons(port);
        if (non_blocking) {
            fcntl(fd, F_SETFL, O_NONBLOCK);
        }
        ret = ::connect(fd, reinterpret_cast<struct sockaddr *>(&sockaddr),
                        sizeof(sockaddr));
        if (ret < 0) {
            if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
                return -1;
            } else {
                syscall_error();
            }
        } else {
            return 0;
        }
    } else if (domain == domain_local) {
        struct sockaddr_un sockaddr;
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sun_family = AF_UNIX;
        memcpy(sockaddr.sun_path, ip.c_str(), ip.size() + 1);
        if (non_blocking) {
            fcntl(fd, F_SETFL, O_NONBLOCK);
        }
        ret = ::connect(fd, reinterpret_cast<struct sockaddr *>(&sockaddr),
                        sizeof(sockaddr));
        if (ret < 0) {
            if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
                return -1;
            } else {
                syscall_error();
            }
        } else {
            return 0;
        }
    } else {
        fatal_error("Protocal not implemented.");
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
string Socket::get_local_ip() const {
    return local_ip;
}
uint16_t Socket::get_local_port() const {
    return local_port;
}
string Socket::get_remote_ip() const {
    return remote_ip;
}
uint16_t Socket::get_remote_port() const {
    return remote_port;
}