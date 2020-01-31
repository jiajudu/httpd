#include "server/threadPoolReactorServer.h"
#include "auxiliary/error.h"
#include "unistd.h"
#include <poll.h>
#include <sys/eventfd.h>
#include <thread>
#include <unordered_set>
ThreadPoolReactorServer::ThreadData::ThreadData() {
    eventfd = ::eventfd(0, EFD_NONBLOCK);
}
void ThreadPoolReactorServer::ThreadData::add_connection(
    shared_ptr<Connection> conn) {
    unique_lock<mutex> g(lock);
    sockets[conn->get_fd()] = conn;
    cond.notify_all();
}
void ThreadPoolReactorServer::ThreadData::assume_has_connection() {
    unique_lock<mutex> g(lock);
    while (sockets.size() == 0) {
        cond.wait(g);
    }
}
void ThreadPoolReactorServer::ThreadData::it(
    function<void(int fd, shared_ptr<Connection> socket)> op) {
    unique_lock<mutex> g(lock);
    for (auto it = sockets.begin(); it != sockets.end(); it++) {
        op(it->first, it->second);
    }
}
shared_ptr<Connection>
ThreadPoolReactorServer::ThreadData::get_connection(int fd) {
    unique_lock<mutex> g(lock);
    return sockets[fd];
}
void ThreadPoolReactorServer::ThreadData::delete_connection(int fd) {
    unique_lock<mutex> g(lock);
    sockets.erase(fd);
}
int ThreadPoolReactorServer::ThreadData::get_eventfd() const {
    return eventfd;
}
ThreadPoolReactorServer::ThreadPoolReactorServer(string &_ip, uint16_t _port,
                                                 int _numThreads)
    : Server(_ip, _port), numThreads(_numThreads) {
}
void ThreadPoolReactorServer::run() {
    if (numThreads <= 0) {
        exit(1);
    }
    vector<thread> threads;
    vector<ThreadData> data(numThreads);
    for (int i = 0; i < numThreads; i++) {
        threads.push_back(
            thread(&ThreadPoolReactorServer::worker_main, this, ref(data[i])));
    }
    listener = make_shared<Listener>(ip, port, 10, true);
    for (size_t i = 0;; i++) {
        shared_ptr<Connection> conn = listener->accept();
        data[i % numThreads].add_connection(conn);
        eventfd_write(data[i % numThreads].get_eventfd(), 1);
    }
}
void ThreadPoolReactorServer::worker_main(ThreadData &data) {
    while (true) {
        data.assume_has_connection();
        vector<struct pollfd> pollfds;
        pollfds.push_back({data.get_eventfd(), POLLIN, 0});
        data.it([&](int fd, shared_ptr<Connection> socket) -> void {
            short int events = POLLIN | POLLPRI | POLLRDHUP;
            if (socket->has_content_to_send()) {
                events |= POLLOUT;
            }
            pollfds.push_back({fd, events, 0});
        });
        int ret = poll(&pollfds[0], pollfds.size(), -1);
        if (ret < 0) {
            syscall_error();
        }
        for (auto &pollfd : pollfds) {
            if (pollfd.fd == data.get_eventfd()) {
                if (pollfd.revents & POLLIN) {
                    eventfd_t count;
                    eventfd_read(data.get_eventfd(), &count);
                }
            } else {
                shared_ptr<Connection> conn = data.get_connection(pollfd.fd);
                if (pollfd.revents & POLLNVAL) {
                    agreement_error("poll invalid fd");
                } else if (pollfd.revents & POLLERR) {
                    data.delete_connection(pollfd.fd);
                    conn->close();
                } else {
                    if (pollfd.revents & POLLIN) {
                        conn->non_blocking_recv();
                        string message;
                        conn->recv(message, decoder);
                        if (message.size() > 0) {
                            onMessage(message, [&](string &s) -> size_t {
                                return conn->send(s);
                            });
                        }
                    }
                    if (pollfd.revents & POLLOUT) {
                        conn->non_blocking_send();
                    }
                    if (pollfd.revents & POLLRDHUP) {
                        data.delete_connection(pollfd.fd);
                        conn->close();
                    }
                }
            }
        }
    }
}
