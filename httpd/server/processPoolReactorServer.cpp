#include "server/processPoolReactorServer.h"
#include "auxiliary/error.h"
#include "multiplexing/multiplexer.h"
#include "multiplexing/poller.h"
#include <algorithm>
#include <iostream>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
ProcessPoolReactorServer::ProcessPoolReactorServer(string &_ip, uint16_t _port,
                                                   int _numProcess)
    : Server(_ip, _port), numProcess(_numProcess) {
}
void ProcessPoolReactorServer::run() {
    if (numProcess <= 0) {
        exit(1);
    }
    vector<FDTransmission> child_fds;
    for (int i = 0; i < numProcess; i++) {
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
    listener = make_shared<Listener>(ip, port, 10, true);
    for (long i = 0;; i++) {
        shared_ptr<Connection> conn = listener->accept();
        child_fds[i % numProcess].send_conn(conn);
        conn->close();
    }
}
void ProcessPoolReactorServer::child_main(FDTransmission &fdt) {
    shared_ptr<Multiplexer> multiplexer = make_shared<Poller>();
    multiplexer->socket_read_callback =
        [&](shared_ptr<Connection> conn) -> void {
        conn->non_blocking_recv();
        string message;
        conn->recv(message, decoder);
        if (message.size() > 0) {
            onMessage(message,
                      [&](string &s) -> size_t { return conn->send(s); });
        }
        multiplexer->mod_connection_fd(conn, true, conn->has_content_to_send());
    };
    multiplexer->socket_write_callback =
        [&](shared_ptr<Connection> conn) -> void {
        conn->non_blocking_send();
        multiplexer->mod_connection_fd(conn, true, conn->has_content_to_send());
    };
    multiplexer->socket_error_callback =
        [&](shared_ptr<Connection> conn) -> void {
        multiplexer->del_connection_fd(conn);
        conn->close();
    };
    multiplexer->socket_hang_up_callback =
        [&](shared_ptr<Connection> conn) -> void {
        multiplexer->del_connection_fd(conn);
        conn->close();
    };
    multiplexer->add_event_fd(fdt.get_fd());
    multiplexer->eventfd_read_callback = [&](int _fd) -> void {
        if (_fd == fdt.get_fd()) {
            shared_ptr<Connection> conn = fdt.recv_conn();
            multiplexer->add_connection_fd(conn, true, false);
        }
    };
    while (true) {
        multiplexer->read();
    }
}
