#include "socket/socket.h"
#include "auxiliary/error.h"
#include "socket/ip.h"
#include <arpa/inet.h>
#include <memory>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
const int Socket::domainINET = AF_INET;
const int Socket::domainINET6 = AF_INET6;
Socket::Socket(int domain_, bool nonBlock_, bool closeExec_)
    : domain(domain_), nonBlock(nonBlock_), closeExec(closeExec_) {
    int type = SOCK_STREAM;
    if (nonBlock) {
        type |= SOCK_NONBLOCK;
    }
    if (closeExec) {
        type |= SOCK_CLOEXEC;
    }
    fd = socket(domain, type, 0);
    if (fd < 0) {
        fatalError();
    }
}
Socket::Socket(int fd_, int domain_, bool nonBlock_, bool closeExec_)
    : domain(domain_), nonBlock(nonBlock_), closeExec(closeExec_), fd(fd_) {
}
int Socket::bind(std::string &ip, uint16_t port) {
    int ret = 0;
    if (domain == domainINET) {
        struct sockaddr_in sockaddr;
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sin_family = domainINET;
        sockaddr.sin_addr.s_addr = inet_ston(ip);
        sockaddr.sin_port = htons(port);
        ret = ::bind(fd, reinterpret_cast<struct sockaddr *>(&sockaddr),
                     sizeof(sockaddr));
        if (ret < 0) {
            fatalError();
        }
    } else if (domain == domainINET6) {
        write(1, "IPv6 not implemented.\n", 23);
        exit(1);
    }
    return ret;
}
int Socket::listen(int backlog) {
    int ret = ::listen(fd, backlog);
    if (ret < 0) {
        fatalError();
    }
    return ret;
}
std::shared_ptr<Socket> Socket::accept() {
    std::shared_ptr<Socket> ret;
    if (domain == domainINET) {
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        int connfd =
            ::accept(fd, reinterpret_cast<struct sockaddr *>(&addr), &addrlen);
        if (connfd < 0) {
            fatalError();
        }
        ret = std::make_shared<Socket>(connfd, domain, nonBlock, closeExec);
    } else if (domain == domainINET6) {
        write(1, "IPv6 not implemented.\n", 23);
        exit(1);
    }
    return ret;
}
ssize_t Socket::recv(char *buf, uint64_t len, bool dontWait, bool waitAll,
                     bool peek, bool oob) {
    int flag = 0;
    if (dontWait) {
        flag |= MSG_DONTWAIT;
    }
    if (waitAll) {
        flag |= MSG_WAITALL;
    }
    if (peek) {
        flag |= MSG_PEEK;
    }
    if (oob) {
        flag |= MSG_OOB;
    }
    ssize_t ret = ::recv(fd, buf, len, flag);
    return ret;
}
ssize_t Socket::send(char *buf, uint64_t len, bool dontWait, bool more,
                     bool oob) {
    int flag = 0;
    if (dontWait) {
        flag |= MSG_DONTWAIT;
    }
    if (more) {
        flag |= MSG_MORE;
    }
    if (oob) {
        flag |= MSG_OOB;
    }
    ssize_t ret = ::send(fd, buf, len, flag);
    return ret;
}
Socket::~Socket() {
    int ret = close(fd);
    if (ret < 0) {
        fatalError();
    }
}