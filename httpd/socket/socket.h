#include "auxiliary/noncopyable.h"
#include <memory>
#include <string>
class Socket : noncopyable {
public:
    Socket(int domain_, bool nonBlock_, bool closeExec_);
    Socket(int fd_, int domain_, bool nonBlock_, bool closeExec_);
    ~Socket();
    int bind(std::string &ip, uint16_t port);
    int listen(int backlog);
    std::shared_ptr<Socket> accept();
    ssize_t recv(char *buf, uint64_t len, bool dontWait = false,
                 bool waitAll = false, bool peek = false, bool oob = false);
    ssize_t send(char *buf, uint64_t len, bool dontWait = false,
                 bool more = false, bool oob = false);
    static const int domainINET;
    static const int domainINET6;

private:
    int domain;
    bool nonBlock;
    bool closeExec;
    int fd;
};