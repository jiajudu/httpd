#pragma once
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
class ProcessPoolReactorServer : public Server {
public:
    ProcessPoolReactorServer(string &ip, uint16_t port, int numProcess);
    void run();

private:
    int numProcess;
    vector<int> child_fds;
    void child_main(int fd);
    int recvFd(int fd);
    void send_conn(int fd, shared_ptr<Connection> conn);
    shared_ptr<Connection> recv_conn(int fd);
};
