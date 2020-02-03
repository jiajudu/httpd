#include "auxiliary/blockingQueue.h"
#include "auxiliary/ip.h"
#include "schedule/multiplexer.h"
#include "schedule/poller.h"
#include "socket/connector.h"
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/eventfd.h>
#include <thread>
#include <unistd.h>
using namespace std;
int main(int argc, char **argv) {
    if (argc != 3) {
        exit(1);
    }
    string ip(argv[1]);
    uint16_t port = static_cast<uint16_t>(stoi(string(argv[2])));
    shared_ptr<Connection> conn = Connector::connect(ip, port);
    int fop = fcntl(1, F_GETFL);
    fcntl(1, F_SETFL, fop | O_NONBLOCK);
    string message;
    bool working = true;
    conn->decode = [](char *s_buf, size_t n_buf) {
        for (char *p = s_buf + n_buf - 1; p >= s_buf; p--) {
            if (*p == '\n') {
                return static_cast<size_t>(p - s_buf + 1);
            }
        }
        return static_cast<size_t>(0);
    };
    shared_ptr<Multiplexer> multiplexer = make_shared<Poller>();
    conn->onClose = [&](shared_ptr<Connection> c) -> void {
        multiplexer->del_connection_fd(c);
    };
    conn->onSendBegin = [&](shared_ptr<Connection> c) -> void {
        multiplexer->mod_connection_fd(c, true, true);
    };
    conn->onSendComplete = [&](shared_ptr<Connection> c) -> void {
        multiplexer->mod_connection_fd(c, true, false);
    };
    multiplexer->add_event_fd(1);
    multiplexer->add_connection_fd(conn, true, false);
    multiplexer->eventfd_read_callback = [&](int fd) -> void {
        if (fd == 1) {
            char buf[1];
            ssize_t size = read(1, buf, 1);
            while (size > 0) {
                if (size > 0) {
                    message.push_back(buf[0]);
                    if (buf[0] == '\n') {
                        conn->send(message);
                        message.clear();
                    }
                } else if (size == 0) {
                    working = false;
                }
                size = read(1, buf, 1);
            }
        }
    };
    multiplexer->socket_read_callback = [&](shared_ptr<Connection> c) {
        c->non_blocking_recv();
        string _m;
        c->recv(_m);
        if (_m.size() > 0) {
            cout << _m;
        }
    };
    multiplexer->socket_write_callback = [&](shared_ptr<Connection> c) -> void {
        c->non_blocking_send();
    };
    multiplexer->socket_error_callback = [&](shared_ptr<Connection> c) -> void {
        multiplexer->del_connection_fd(c);
        c->shutdown();
    };
    multiplexer->socket_hang_up_callback =
        [&](shared_ptr<Connection> c) -> void { c->close(); };
    while (conn->active() && working) {
        multiplexer->read();
    }
    return 0;
}