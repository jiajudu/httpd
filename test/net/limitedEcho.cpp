#include "net/server/server.h"
#include "net/service/service.h"
#include "net/socket/socket.h"
#include "net/util/noncopyable.h"
#include <functional>
#include <memory>
class LimitedEcho : public Service {
public:
    void onMessage(shared_ptr<Connection> conn, string &input_message) {
        conn->send(input_message);
    }
    void onConnection(shared_ptr<Connection> conn) {
        conn->set_deactivation(10);
    }
};
int main(int argc, char **argv) {
    if (argc != 3) {
        return 1;
    }
    uint16_t port = static_cast<uint16_t>(stoi(argv[2]));
    ServerOption server_option;
    shared_ptr<Service> service = make_shared<LimitedEcho>();
    shared_ptr<Server> server =
        get_server(service, argv[1], "127.0.0.1", port, server_option);
    server->run();
    return 0;
}