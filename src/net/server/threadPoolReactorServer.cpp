#include "net/server/threadPoolReactorServer.h"
#include "net/schedule/connectionPool.h"
#include "net/schedule/epoller.h"
#include "net/schedule/eventPool.h"
#include "net/schedule/poller.h"
#include "net/schedule/scheduler.h"
#include "net/schedule/timerPool.h"
#include "net/util/blockingQueue.h"
#include "net/util/error.h"
#include <iostream>
#include <poll.h>
#include <sys/eventfd.h>
#include <thread>
#include <unistd.h>
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
        queues[i % option.thread_number].push(conn);
        eventfd_write(event_fds[i % option.thread_number], 1);
    }
}
void ThreadPoolReactorServer::worker_main(Queue<shared_ptr<Connection>> &conn_q,
                                          int event_fd) {
    shared_ptr<Scheduler> scheduler;
    if (option.scheduler == "poll") {
        scheduler = make_shared<Poller>();
    } else {
        scheduler = make_shared<EPoller>();
    }
    shared_ptr<EventPool> event_pool = scheduler->events;
    shared_ptr<ConnectionPool> connection_pool = scheduler->connections;
    shared_ptr<TimerPool> timer_pool = scheduler->timers;
    timer_pool->enable_deactivation();
    shared_ptr<ConnectionEvent> conn_ev = make_shared<ConnectionEvent>();
    conn_ev->onConnection = [this](shared_ptr<Connection> conn) -> void {
        service->onConnection(conn);
    };
    conn_ev->onMessage = [this](shared_ptr<Connection> conn,
                                string &message) -> void {
        service->onMessage(conn, message);
    };
    conn_ev->onSendComplete = [this](shared_ptr<Connection> conn) -> void {
        service->onSendComplete(conn);
    };
    conn_ev->onDisconnect = [this](shared_ptr<Connection> conn) -> void {
        service->onDisconnect(conn);
    };
    event_pool->add_event(
        event_fd,
        [event_fd, &connection_pool, &conn_q, this, &conn_ev]() -> void {
            queue<shared_ptr<Connection>> conns;
            conn_q.pop_all(conns);
            while (conns.size() > 0) {
                shared_ptr<Connection> conn = conns.front();
                conns.pop();
                if (connection_pool->size() >=
                    static_cast<size_t>(option.max_connection_number)) {
                    conn->close();
                } else {
                    connection_pool->add_connection(conn, conn_ev);
                    conn->set_deactivation(option.timeout);
                }
            }
        });
    while (true) {
        scheduler->read();
    }
}