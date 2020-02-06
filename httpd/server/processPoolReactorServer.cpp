#include "server/processPoolReactorServer.h"
#include "auxiliary/error.h"
#include "fastcgi/fastcgi.h"
#include "schedule/connectionPool.h"
#include "schedule/eventPool.h"
#include "schedule/multiplexer.h"
#include "schedule/poller.h"
#include "schedule/timerPool.h"
#include "service/service.h"
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
    shared_ptr<Multiplexer> multiplexer = make_shared<Poller>();
    shared_ptr<EventPool> event_pool = multiplexer->events;
    shared_ptr<ConnectionPool> connection_pool = multiplexer->connections;
    shared_ptr<TimerPool> timer = multiplexer->timers;
    string fcgi_ip("127.0.0.1");
    shared_ptr<FastCGI> fcgi = make_shared<FastCGI>(multiplexer, fcgi_ip, 8000);
    service->tl() = fcgi;
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
        [&fdt, connection_pool, this, timer, &conn_ev]() -> void {
            shared_ptr<Connection> conn = fdt.recv_conn();
            if (connection_pool->size() >=
                static_cast<size_t>(option.max_connection_number)) {
                conn->close();
            } else {
                connection_pool->add_connection(conn, conn_ev);
            }
        },
        false);
    while (true) {
        multiplexer->read();
    }
}
