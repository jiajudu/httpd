#include "net/server/server.h"
#include "net/service/service.h"
#include "net/util/tm.h"
class DayTime : public Service {
public:
    void onConnection(shared_ptr<Connection> conn) {
        string t = get_time_fmt();
        conn->send(t);
        conn->close();
    }
};
int main(int argc, char **argv) {
    if (argc != 3) {
        return 1;
    }
    uint16_t port = static_cast<uint16_t>(stoi(argv[2]));
    ServerOption server_option;
    shared_ptr<Service> service = make_shared<DayTime>();
    shared_ptr<Server> server =
        get_server(service, argv[1], "127.0.0.1", port, server_option);
    server->run();
    return 0;
}