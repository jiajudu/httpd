#include "multiplexing/multiplexer.h"
#include <unordered_set>
class Poller : public Multiplexer {
public:
    void add_connection_fd(shared_ptr<Connection> connection, bool event_read,
                           bool event_write);
    void mod_connection_fd(shared_ptr<Connection> connection, bool event_read,
                           bool event_write);
    void del_connection_fd(shared_ptr<Connection> connection);
    void add_event_fd(int fd);
    void read();

private:
    unordered_map<int, pair<shared_ptr<Connection>, int>> sockets;
    unordered_set<int> events;
};