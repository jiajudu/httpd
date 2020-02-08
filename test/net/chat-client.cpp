#include "net/schedule/connectionPool.h"
#include "net/schedule/connectorPool.h"
#include "net/schedule/eventPool.h"
#include "net/schedule/poller.h"
#include "net/schedule/scheduler.h"
#include "net/util/blockingQueue.h"
#include "net/util/ip.h"
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
    uint16_t port = static_cast<uint16_t>(stoi(argv[2]));
    int fop = fcntl(0, F_GETFL);
    fcntl(0, F_SETFL, fop | O_NONBLOCK);
    string message;
    bool working = true;
    shared_ptr<Scheduler> scheduler = make_shared<Poller>();
    shared_ptr<EventPool> event_pool = scheduler->events;
    shared_ptr<ConnectionPool> connection_pool = scheduler->connections;
    shared_ptr<ConnectorPool> connector_pool = scheduler->connectors;
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
    shared_ptr<ConnectionEvent> conn_ev = make_shared<ConnectionEvent>();
    conn_ev->onMessage = [](shared_ptr<Connection> conn, string &m) -> void {
        (void)conn;
        if (m.size() > 0) {
            cout << m;
        }
    };
    conn_ev->onDisconnect = [&working](shared_ptr<Connection> conn) {
        (void)conn;
        working = false;
    };
    connector_pool->connect(
        argv[1], port,
        [&c, &connection_pool, &conn_ev](shared_ptr<Connection> conn) -> void {
            c = conn;
            cout << "Connection Success\n";
            conn->decode = [](char *s_buf, size_t n_buf) {
                for (char *p = s_buf + n_buf - 1; p >= s_buf; p--) {
                    if (*p == '\n') {
                        return static_cast<size_t>(p - s_buf + 1);
                    }
                }
                return static_cast<size_t>(0);
            };
            connection_pool->add_connection(conn, conn_ev);
        },
        [](shared_ptr<Connection> conn) -> void {
            (void)conn;
            cout << "Connection Error\n";
        });
    cout << "Start connection\n";
    while (working) {
        scheduler->read();
    }
    return 0;
}