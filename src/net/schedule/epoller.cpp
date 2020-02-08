#include "net/schedule/epoller.h"
#include "net/util/error.h"
#include <sys/epoll.h>
EPoller::EPoller() {
    epoll_fd = epoll_create(1);
    if (epoll_fd < 0) {
        syscall_error();
    }
}
void EPoller::add_fd(int fd, bool event_read, bool event_write,
                     shared_ptr<EventHandler> eh) {
    short es = 0;
    if (event_read) {
        es |= (EPOLLIN | EPOLLPRI | EPOLLRDHUP);
    }
    if (event_write) {
        es |= EPOLLOUT;
    }
    fds[fd] = pair<short, shared_ptr<EventHandler>>(es, eh);
    epoll_event e;
    e.events = es;
    e.data.fd = fd;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &e);
    if (ret < 0) {
        syscall_error();
    }
}
void EPoller::mod_fd(int fd, bool event_read, bool event_write) {
    short es = 0;
    if (event_read) {
        es |= (EPOLLIN | EPOLLPRI | EPOLLRDHUP);
    }
    if (event_write) {
        es |= EPOLLOUT;
    }
    fds[fd].first = es;
    epoll_event e;
    e.events = es;
    e.data.fd = fd;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &e);
    if (ret < 0) {
        syscall_error();
    }
}
void EPoller::del_fd(int fd) {
    fds.erase(fd);
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
    if (ret < 0) {
        syscall_error();
    }
}
void EPoller::read() {
    vector<struct epoll_event> evs(fds.size());
    int ret = epoll_wait(epoll_fd, &evs[0], static_cast<int>(evs.size()), -1);
    if (ret < 0) {
        syscall_error();
    }
    for (int i = 0; i < ret; i++) {
        int fd = evs[i].data.fd;
        if (fds.find(fd) != fds.end()) {
            shared_ptr<EventHandler> eh = fds[fd].second;
            if (evs[i].events & EPOLLERR) {
                if (eh->error) {
                    eh->error(fd);
                }
            } else {
                if ((evs[i].events & EPOLLHUP) &&
                    (!(evs[i].events & EPOLLIN)) && eh->close) {
                    eh->close(fd);
                }
                if (((evs[i].events & EPOLLIN) ||
                     (evs[i].events & EPOLLRDHUP)) &&
                    eh->read) {
                    eh->read(fd);
                }
                if ((evs[i].events & EPOLLOUT) && eh->write) {
                    eh->write(fd);
                }
            }
        }
    }
}
