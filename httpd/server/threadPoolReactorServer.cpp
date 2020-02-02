#include "server/threadPoolReactorServer.h"
#include "auxiliary/blockingQueue.h"
#include "auxiliary/error.h"
#include "multiplexing/multiplexer.h"
#include "multiplexing/poller.h"
#include "unistd.h"
#include <poll.h>
#include <sys/eventfd.h>
#include <thread>
#include <unordered_set>
ThreadPoolReactorServer::ThreadPoolReactorServer(shared_ptr<Service> _service,
                                                 string &_ip, uint16_t _port,
                                                 int _numThreads)
    : Server(_service, _ip, _port), numThreads(_numThreads) {
}
void ThreadPoolReactorServer::run() {
    if (numThreads <= 0) {
        exit(1);
    }
    vector<thread> threads;
    vector<Queue<shared_ptr<Connection>>> queues(numThreads);
    vector<int> event_fds;
    for (int i = 0; i < numThreads; i++) {
        event_fds.push_back(eventfd(0, EFD_NONBLOCK));
        threads.push_back(thread(&ThreadPoolReactorServer::worker_main, this,
                                 ref(queues[i]), event_fds[i]));
    }
    listener = make_shared<Listener>(ip, port, 10, true);
    for (size_t i = 0;; i++) {
        shared_ptr<Connection> conn = listener->accept();
        queues[i % numThreads].push(conn);
        eventfd_write(event_fds[i % numThreads], 1);
    }
}
void ThreadPoolReactorServer::worker_main(Queue<shared_ptr<Connection>> &conn_q,
                                          int event_fd) {
    shared_ptr<Multiplexer> multiplexer = make_shared<Poller>();
    auto connection_close = [&](shared_ptr<Connection> conn) -> void {
        multiplexer->del_connection_fd(conn);
        service->onDisconnect(conn);
    };
    auto connection_send_begin = [&](shared_ptr<Connection> conn) -> void {
        multiplexer->mod_connection_fd(conn, true, true);
    };
    auto connection_send_end = [&](shared_ptr<Connection> conn) -> void {
        multiplexer->mod_connection_fd(conn, true, false);
        service->onSendComplete(conn);
    };
    multiplexer->add_event_fd(event_fd);
    multiplexer->eventfd_read_callback = [&](int _fd) -> void {
        if (_fd == event_fd) {
            eventfd_t e;
            eventfd_read(event_fd, &e);
            queue<shared_ptr<Connection>> conns;
            conn_q.pop_all(conns);
            while (conns.size() > 0) {
                shared_ptr<Connection> conn = conns.front();
                conns.pop();
                multiplexer->add_connection_fd(conn, true, false);
                conn->onClose = connection_close;
                conn->onSendBegin = connection_send_begin;
                conn->onSendComplete = connection_send_end;
                service->onConnection(conn);
            }
        }
    };
    multiplexer->socket_read_callback =
        [&](shared_ptr<Connection> conn) -> void {
        conn->non_blocking_recv();
        string message;
        conn->recv(message);
        if (message.size() > 0) {
            service->onMessage(conn, message);
        }
    };
    multiplexer->socket_write_callback =
        [&](shared_ptr<Connection> conn) -> void { conn->non_blocking_send(); };
    multiplexer->socket_error_callback =
        [&](shared_ptr<Connection> conn) -> void {
        multiplexer->del_connection_fd(conn);
        service->onDisconnect(conn);
        conn->shutdown();
    };
    multiplexer->socket_hang_up_callback =
        [&](shared_ptr<Connection> conn) -> void { conn->close(); };
    while (true) {
        multiplexer->read();
    }
}
