#pragma once
#include "auxiliary/noncopyable.h"
#include <memory>
#include <string>
using std::shared_ptr;
using std::string;
class Socket : Noncopyable {
public:
    Socket(int domain_, bool nonBlock_, bool closeExec_);
    Socket(int fd_, int domain_, bool nonBlock_, bool closeExec_);
    int bind(string &ip, uint16_t port);
    int listen(int backlog);
    shared_ptr<Socket> accept();
    int close();
    ssize_t _recv(char *buf, ssize_t len);
    ssize_t recv(char *buf, ssize_t len, bool dontWait = false,
                 bool waitAll = false, bool peek = false, bool oob = false);
    ssize_t _send(char *buf, ssize_t len);
    ssize_t send(char *buf, ssize_t len, bool dontWait = false,
                 bool more = false, bool oob = false);
    static const int domainINET;
    static const int domainINET6;
    static const int domainLocal;
    int getDomain() const;
    bool getNonBlock() const;
    bool getCloseExec() const;
    int getFd() const;

private:
    int domain;
    bool nonBlock;
    bool closeExec;
    int fd;
};