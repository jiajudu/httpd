#pragma once
#include "auxiliary/blockingQueue.h"
#include "auxiliary/noncopyable.h"
#include "server/server.h"
#include "socket/socket.h"
#include <functional>
#include <memory>
#include <string>
#include <thread>
class ThreadPoolReactorServer : public Server {
public:
    ThreadPoolReactorServer(string &ip, uint16_t port, int numThreads);
    void run();

private:
    class ThreadData {
    public:
        ThreadData();
        void add_connection(shared_ptr<Connection> conn);
        void assume_has_connection();
        void it(function<void(int fd, shared_ptr<Connection> socket)> op);
        shared_ptr<Connection> get_connection(int fd);
        void delete_connection(int fd);
        int get_eventfd() const;

    private:
        unordered_map<int, shared_ptr<Connection>> sockets;
        mutex lock;
        condition_variable cond;
        int eventfd;
    };
    int numThreads;
    void worker_main(ThreadData &thread_data);
};