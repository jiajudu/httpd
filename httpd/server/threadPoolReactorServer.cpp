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
                                                 ServerOption &server_option)
    : Server(_service, _ip, _port, server_option) {
}
void ThreadPoolReactorServer::run() {
    if (option.thread_number <= 0) {
        exit(1);
    }
    vector<thread> threads;
    vector<Queue<shared_ptr<Connection>>> queues(option.thread_number);
    vector<int> event_fds;
    for (int i = 0; i < option.thread_number; i++) {
        event_fds.push_back(eventfd(0, EFD_NONBLOCK));
        threads.push_back(thread(&ThreadPoolReactorServer::worker_main, this,
                                 ref(queues[i]), event_fds[i]));
    }
    listener = make_shared<Listener>(ip, port, 10, true);
    for (size_t i = 0;; i++) {
        shared_ptr<Connection> conn = listener->accept();
        bool allow = increase_connection_counter();
        if (allow) {
            queues[i % option.thread_number].push(conn);
            eventfd_write(event_fds[i % option.thread_number], 1);
        } else {
            conn->close();
        }
    }
}
void ThreadPoolReactorServer::worker_main(Queue<shared_ptr<Connection>> &conn_q,
                                          int event_fd) {
    shared_ptr<Multiplexer> multiplexer = make_shared<Poller>();
    auto connection_close = [&](shared_ptr<Connection> conn) -> void {
        multiplexer->del_connection_fd(conn);
        service->onDisconnect(conn);
        decrease_connection_counter();
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
        decrease_connection_counter();
    };
    multiplexer->socket_hang_up_callback =
        [&](shared_ptr<Connection> conn) -> void { conn->close(); };
    while (true) {
        multiplexer->read();
    }
}
bool ThreadPoolReactorServer::increase_connection_counter() {
    lock_guard<mutex> g(mutex);
    if (active_connection_number < option.max_connection_number) {
        active_connection_number++;
        return true;
    } else {
        return false;
    }
}
bool ThreadPoolReactorServer::decrease_connection_counter() {
    lock_guard<mutex> g(mutex);
    active_connection_number--;
    return true;
}