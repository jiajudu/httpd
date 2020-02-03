#include "auxiliary/blockingQueue.h"
#include "auxiliary/ip.h"
#include "schedule/clientConnectionPool.h"
#include "schedule/eventPool.h"
#include "schedule/multiplexer.h"
#include "schedule/poller.h"
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
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
    int fop = fcntl(0, F_GETFL);
    fcntl(0, F_SETFL, fop | O_NONBLOCK);
    string message;
    bool working = true;
    shared_ptr<Multiplexer> multiplexer = make_shared<Poller>();
    shared_ptr<EventPool> event_pool = make_shared<EventPool>(multiplexer);
    shared_ptr<ClientConnectionPool> connection_pool =
        make_shared<ClientConnectionPool>(multiplexer);
    shared_ptr<Connection> c;
    event_pool->add_event(
        0,
        [&message, &working, &c]() -> void {
            char buf[1];
            ssize_t size = read(0, buf, 1);
            while (size >= 0) {
                if (size > 0) {
                    message.push_back(buf[0]);
                    if (buf[0] == '\n') {
                        if (c) {
                            c->send(message);
                        }
                        message.clear();
                    }
                } else if (size == 0) {
                    working = false;
                }
                size = read(0, buf, 1);
            }
        },
        false);
    connection_pool->onConnection = [&c](shared_ptr<Connection> conn) -> void {
        c = conn;
        if (c) {
            cout << "Connection Success\n";
        }
        conn->decode = [](char *s_buf, size_t n_buf) {
            for (char *p = s_buf + n_buf - 1; p >= s_buf; p--) {
                if (*p == '\n') {
                    return static_cast<size_t>(p - s_buf + 1);
                }
            }
            return static_cast<size_t>(0);
        };
    };
    connection_pool->onMessage = [](shared_ptr<Connection> conn,
                                    string &m) -> void {
        (void)conn;
        if (m.size() > 0) {
            cout << m;
        }
    };
    connection_pool->onDisconnect = [&c,
                                     &working](shared_ptr<Connection> conn) {
        if (conn == c) {
            working = false;
        }
    };
    connection_pool->connect(ip, port);
    cout << "Start connection\n";
    while (working) {
        multiplexer->read();
    }
    return 0;
}