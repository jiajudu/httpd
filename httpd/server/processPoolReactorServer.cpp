#include "server/processPoolReactorServer.h"
#include "auxiliary/error.h"
#include "schedule/connectionPool.h"
#include "schedule/eventPool.h"
#include "schedule/multiplexer.h"
#include "schedule/poller.h"
#include "schedule/timerPool.h"
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
    shared_ptr<EventPool> event_pool = make_shared<EventPool>(multiplexer);
    shared_ptr<ConnectionPool> connection_pool =
        make_shared<ConnectionPool>(multiplexer, service);
    shared_ptr<TimerPool> timer = make_shared<TimerPool>(multiplexer);
    event_pool->add_event(
        fdt.get_fd(),
        [&fdt, connection_pool, this, timer]() -> void {
            shared_ptr<Connection> conn = fdt.recv_conn();
            if (connection_pool->size() >=
                static_cast<size_t>(option.max_connection_number)) {
                conn->close();
            } else {
                conn->timer = timer;
                connection_pool->add_connection(conn);
            }
        },
        false);
    while (true) {
        multiplexer->read();
    }
}
