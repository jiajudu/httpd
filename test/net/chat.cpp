#include "net/server/server.h"
#include "net/service/service.h"
#include "net/socket/socket.h"
#include "net/util/noncopyable.h"
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>
class Chat : public Service {
public:
    void onConnection(shared_ptr<Connection> conn) {
        lock_guard<mutex> g(lock);
        conns.insert(conn);
    }
    void onMessage(shared_ptr<Connection> conn, string &input_message) {
        lock_guard<mutex> g(lock);
        for (auto c : conns) {
            if (c != conn) {
                c->send(input_message);
            }
        }
    }
    void onDisconnect(shared_ptr<Connection> conn) {
        lock_guard<mutex> g(lock);
        conns.erase(conn);
    }

private:
    unordered_set<shared_ptr<Connection>> conns;
    mutex lock;
};
int main(int argc, char **argv) {
    if (argc != 3) {
        return 1;
    }
    uint16_t port = static_cast<uint16_t>(stoi(argv[2]));
    ServerOption server_option;
    shared_ptr<Service> service = make_shared<Chat>();
    shared_ptr<Server> server =
        get_server(service, argv[1], "127.0.0.1", port, server_option);
    server->run();
    return 0;
}
