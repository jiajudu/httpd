#include "net/server/server.h"
#include "net/service/service.h"
#include "net/util/tm.h"
class Timestamp : public Service {
public:
    void onConnection(shared_ptr<Connection> conn) {
        timeval t = get_time();
        time_t sec = t.tv_sec;
        string s(reinterpret_cast<char *>(&sec),
                 reinterpret_cast<char *>(&sec) + sizeof(sec));
        reverse(s.begin(), s.end());
        conn->send(s);
        conn->close();
    }
};
int main(int argc, char **argv) {
    if (argc != 3) {
        return 1;
    }
    uint16_t port = static_cast<uint16_t>(stoi(argv[2]));
    ServerOption server_option;
    shared_ptr<Service> service = make_shared<Timestamp>();
    shared_ptr<Server> server =
        get_server(service, argv[1], "127.0.0.1", port, server_option);
    server->run();
    return 0;
}