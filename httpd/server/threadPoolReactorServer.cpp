#include "server/threadPoolReactorServer.h"
#include "auxiliary/blockingQueue.h"
#include "auxiliary/error.h"
#include "schedule/connectionPool.h"
#include "schedule/eventPool.h"
#include "schedule/multiplexer.h"
#include "schedule/poller.h"
#include "schedule/timerPool.h"
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
    shared_ptr<EventPool> event_pool = make_shared<EventPool>(multiplexer);
    shared_ptr<ConnectionPool> connection_pool =
        make_shared<ConnectionPool>(multiplexer);
    shared_ptr<TimerPool> timer = make_shared<TimerPool>(multiplexer);
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
                }
            }
        });
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