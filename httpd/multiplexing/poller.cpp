#include "multiplexing/poller.h"
#include "auxiliary/error.h"
#include <poll.h>
void Poller::add_connection_fd(shared_ptr<Connection> connection,
                               bool event_read, bool event_write) {
    sockets[connection->get_fd()] = pair<shared_ptr<Connection>, int>(
        connection, (event_read ? 1 : 0) | (event_write ? 2 : 0));
}
void Poller::mod_connection_fd(shared_ptr<Connection> connection,
                               bool event_read, bool event_write) {
    sockets[connection->get_fd()].second =
        (event_read ? 1 : 0) | (event_write ? 2 : 0);
}
void Poller::del_connection_fd(shared_ptr<Connection> connection) {
    sockets.erase(connection->get_fd());
}
void Poller::add_event_fd(int fd) {
    events.insert(fd);
}
void Poller::read() {
    vector<struct pollfd> pfds;
    for (auto it = sockets.begin(); it != sockets.end(); it++) {
        short int event = 0;
        if (it->second.second & 1) {
            event |= (POLLIN | POLLPRI | POLLRDHUP);
        }
        if (it->second.second & 2) {
            event |= POLLOUT;
        }
        pfds.push_back({it->first, event, 0});
    }
    for (int eventfd : events) {
        pfds.push_back({eventfd, POLLIN, 0});
    }
    int ret = poll(&pfds[0], pfds.size(), -1);
    if (ret < 0) {
        syscall_error();
    }
    for (auto &pfd : pfds) {
        if (events.find(pfd.fd) != events.end()) {
            if ((pfd.revents & POLLIN) && eventfd_read_callback) {
                eventfd_read_callback(pfd.fd);
            }
        } else if (sockets.find(pfd.fd) != sockets.end()) {
            shared_ptr<Connection> conn = sockets[pfd.fd].first;
            if (pfd.revents & POLLNVAL) {
                agreement_error("poll invalid fd");
            } else if (pfd.revents & POLLERR) {
                if (socket_error_callback) {
                    socket_error_callback(conn);
                }
            } else {
                if ((pfd.revents & POLLIN) && socket_read_callback) {
                    socket_read_callback(conn);
                }
                if ((pfd.revents & POLLOUT) && socket_write_callback) {
                    socket_write_callback(conn);
                }
                if ((pfd.revents & POLLRDHUP) && socket_hang_up_callback) {
                    socket_hang_up_callback(conn);
                }
            }
        }
    }
}
size_t Poller::get_socket_number() {
    return sockets.size();
}