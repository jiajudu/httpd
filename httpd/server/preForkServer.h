#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
class ChildProcess {
public:
    ChildProcess(int _fd);
    int fd;
    bool busy;
};
class PreForkServer : public Server {
public:
    PreForkServer(string &ip, uint16_t port, int numProcess);
    void run();

private:
    int numProcess;
    vector<ChildProcess> childs;
    void childMain(int fd);
    int recvFd(int fd);
    size_t getAvailableProcess();
    void sendConn(int fd, shared_ptr<Connection> conn);
    shared_ptr<Connection> recvConn(int fd);
};
