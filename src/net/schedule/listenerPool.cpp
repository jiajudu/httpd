#include "net/schedule/listenerPool.h"
#include "net/schedule/scheduler.h"
ListenerPool::ListenerPool() {
    eh = make_shared<EventHandler>();
    eh->read = bind(&ListenerPool::read_callback, this, _1);
}
void ListenerPool::add_listener(
    shared_ptr<Listener> listener,
    function<void(shared_ptr<Connection>)> onConnection) {
    listener->pool = shared_from_this();
    listeners[listener->get_fd()] = {listener, onConnection};
    scheduler->add_fd(listener->get_fd(), true, false, eh);
}
void ListenerPool::read_callback(int fd) {
    if (listeners.find(fd) != listeners.end()) {
        auto listener = listeners[fd].first;
        auto callback = listeners[fd].second;
        shared_ptr<Connection> conn = listener->accept();
        callback(conn);
    }
}
void ListenerPool::remove_listener(shared_ptr<Listener> listener) {
    scheduler->del_fd(listener->get_fd());
}
