#include "server/threadPoolReactorServer.h"
#include "auxiliary/blockingQueue.h"
#include "auxiliary/error.h"
#include "schedule/multiplexer.h"
#include "schedule/poller.h"
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
    listener = make_shared<Listener>(ip, port, 10);
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
    multiplexer->add_event_fd(event_fd);
    shared_ptr<Timer> timer = make_shared<Timer>();
    timer->add_timer_callback = [&](int fd) -> void {
        multiplexer->add_event_fd(fd);
    };
    timer->remove_timer_callback = [&](int fd) -> void {
        multiplexer->remove_event_fd(fd);
    };
    multiplexer->add_event_fd(timer->get_sfd());
    multiplexer->eventfd_read_callback = [&](int _fd) -> void {
        if (_fd == event_fd) {
            eventfd_t e;
            eventfd_read(event_fd, &e);
            queue<shared_ptr<Connection>> conns;
            conn_q.pop_all(conns);
            while (conns.size() > 0) {
                shared_ptr<Connection> conn = conns.front();
                conns.pop();
                if (multiplexer->get_socket_number() >=
                    static_cast<size_t>(option.max_connection_number)) {
                    conn->close();
                } else {
                    conn->timer = timer;
                    multiplexer->add_connection_fd(conn, true, false);
                    conn->onClose = [&](shared_ptr<Connection> _c) -> void {
                        multiplexer->del_connection_fd(_c);
                        service->onDisconnect(_c);
                    };
                    conn->onSendBegin = [&](shared_ptr<Connection> _c) -> void {
                        multiplexer->mod_connection_fd(_c, true, true);
                    };
                    conn->onSendComplete =
                        [&](shared_ptr<Connection> _c) -> void {
                        multiplexer->mod_connection_fd(_c, true, false);
                        service->onSendComplete(_c);
                    };
                    service->onConnection(conn);
                }
            }
        } else {
            timer->event(_fd);
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