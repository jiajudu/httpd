#include "net/server/processPoolReactorServer.h"
#include "net/schedule/connectionPool.h"
#include "net/schedule/epoller.h"
#include "net/schedule/eventPool.h"
#include "net/schedule/poller.h"
#include "net/schedule/scheduler.h"
#include "net/schedule/timerPool.h"
#include "net/service/service.h"
#include "net/util/error.h"
#include <algorithm>
#include <iostream>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
ProcessPoolReactorServer::ProcessPoolReactorServer(shared_ptr<Service> _service,
                                                   string &_ip, uint16_t _port,
                                                   ServerOption &server_option)
    : Server(_service, _ip, _port, server_option) {
}
void ProcessPoolReactorServer::run() {
    if (option.process_number <= 0) {
        exit(1);
    }
    vector<FDTransmission> child_fds;
    for (int i = 0; i < option.process_number; i++) {
        FDTransmission fdt;
        int pid = fork();
        if (pid < 0) {
            syscall_error();
        }
        if (pid > 0) {
            fdt.parent();
            child_fds.push_back(fdt);
        } else {
            for (int j = 0; j < i; j++) {
                child_fds[j].child();
            }
            fdt.child();
            child_main(fdt);
        }
    }
    listener = make_shared<Listener>(ip, port, 10);
    for (long i = 0;; i++) {
        shared_ptr<Connection> conn = listener->accept();
        child_fds[i % option.process_number].send_conn(conn);
        conn->close();
    }
}
void ProcessPoolReactorServer::child_main(FDTransmission &fdt) {
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
        fdt.get_fd(),
        [&fdt, connection_pool, this, &conn_ev]() -> void {
            shared_ptr<Connection> conn = fdt.recv_conn();
            if (connection_pool->size() >=
                static_cast<size_t>(option.max_connection_number)) {
                conn->close();
            } else {
                connection_pool->add_connection(conn, conn_ev);
                conn->set_deactivation(option.timeout);
            }
        },
        false);
    while (true) {
        scheduler->read();
    }
}
