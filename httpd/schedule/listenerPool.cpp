#include "schedule/listenerPool.h"
ListenerPool::ListenerPool(shared_ptr<Multiplexer> _multiplexer,
                           shared_ptr<Listener> _listener)
    : multiplexer(_multiplexer), listener(_listener) {
    eh = make_shared<EventHandler>();
    eh->read = [this](int fd) {
        if (fd == listener->get_fd()) {
            shared_ptr<Connection> conn = listener->accept();
            if (onConnection) {
                onConnection(conn);
            }
        }
    };
    listener->onClose = [this]() -> void {
        multiplexer->del_fd(listener->get_fd());
    };
    multiplexer->add_fd(listener->get_fd(), true, false, eh);
}
