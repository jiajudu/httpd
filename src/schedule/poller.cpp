#include "schedule/poller.h"
#include "auxiliary/error.h"
#include <poll.h>
void Poller::add_fd(int fd, bool event_read, bool event_write,
                    shared_ptr<EventHandler> eh) {
    fds[fd] = pair<short, shared_ptr<EventHandler>>(
        static_cast<short>((event_read ? (POLLIN | POLLPRI | POLLRDHUP) : 0) |
                           (event_write ? POLLOUT : 0)),
        eh);
}
void Poller::mod_fd(int fd, bool event_read, bool event_write) {
    fds[fd].first =
        static_cast<short>((event_read ? (POLLIN | POLLPRI | POLLRDHUP) : 0) |
                           (event_write ? POLLOUT : 0));
}
void Poller::del_fd(int fd) {
    fds.erase(fd);
}
void Poller::read() {
    vector<struct pollfd> pfds;
    for (auto it = fds.begin(); it != fds.end(); it++) {
        pfds.push_back({it->first, it->second.first, 0});
    }
    int ret = poll(&pfds[0], pfds.size(), -1);
    if (ret < 0) {
        syscall_error();
    }
    for (auto &pfd : pfds) {
        int fd = pfd.fd;
        if (fds.find(fd) != fds.end()) {
            shared_ptr<EventHandler> eh = fds[fd].second;
            if (pfd.revents & POLLNVAL) {
                fatal_error("poll invalid fd");
            } else if (pfd.revents & POLLERR) {
                if (eh->error) {
                    eh->error(fd);
                }
            } else {
                if ((pfd.revents & POLLHUP) && (!(pfd.revents & POLLIN)) &&
                    eh->close) {
                    eh->close(fd);
                }
                if (((pfd.revents & POLLIN) || (pfd.revents & POLLRDHUP)) &&
                    eh->read) {
                    eh->read(fd);
                }
                if ((pfd.revents & POLLOUT) && eh->write) {
                    eh->write(fd);
                }
            }
        }
    }
}
