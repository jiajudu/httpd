#pragma once
#include "auxiliary/noncopyable.h"
#include "auxiliary/std.h"
#include <memory>
#include <string>
class Socket : Noncopyable {
public:
    Socket(int domain_, bool nonBlock_, bool closeExec_);
    Socket(int fd_, int domain_, bool nonBlock_, bool closeExec_);
    int bind(string &ip, uint16_t port);
    int listen(int backlog);
    shared_ptr<Socket> accept();
    int close();
    ssize_t recv(vector<char> &buf, ssize_t size);
    ssize_t send(vector<char> &buf, ssize_t size);
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
    shared_ptr<vector<char>> readBuf;
    shared_ptr<vector<char>> writeBuf;
    ssize_t _recv(vector<char> &buf, ssize_t size, bool dontWait, bool waitAll, bool peek,
                  bool oob);
    ssize_t _send(vector<char> &buf, ssize_t size, bool dontWait, bool more, bool oob);
};