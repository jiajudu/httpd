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
    void remove_event_fd(int fd);
    void read();
    size_t get_socket_number();

private:
    unordered_map<int, pair<shared_ptr<Connection>, int>> sockets;
    unordered_set<int> events;
};