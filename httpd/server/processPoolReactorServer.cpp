#include "server/processPoolReactorServer.h"
#include "auxiliary/error.h"
#include "schedule/multiplexer.h"
#include "schedule/poller.h"
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
    multiplexer->add_event_fd(fdt.get_fd());
    shared_ptr<Timer> timer = make_shared<Timer>();
    timer->add_timer_callback = [&](int fd) -> void {
        multiplexer->add_event_fd(fd);
    };
    timer->remove_timer_callback = [&](int fd) -> void {
        multiplexer->remove_event_fd(fd);
    };
    multiplexer->add_event_fd(timer->get_sfd());
    multiplexer->eventfd_read_callback = [&](int _fd) -> void {
        if (_fd == fdt.get_fd()) {
            shared_ptr<Connection> conn = fdt.recv_conn();
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
                conn->onSendComplete = [&](shared_ptr<Connection> _c) -> void {
                    multiplexer->mod_connection_fd(_c, true, false);
                    service->onSendComplete(_c);
                };
                service->onConnection(conn);
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
