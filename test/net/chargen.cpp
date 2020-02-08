#include "net/server/server.h"
#include "net/service/service.h"
#include "net/socket/socket.h"
#include "net/util/noncopyable.h"
#include <functional>
#include <memory>
class Chargen : public Service {
public:
    Chargen() {
        for (int i = 0; i < 128; i++) {
            chars.push_back(static_cast<char>(i));
        }
    }
    void onConnection(shared_ptr<Connection> conn) {
        conn->send(chars);
    }
    void onSendComplete(shared_ptr<Connection> conn) {
        conn->send(chars);
    }

private:
    string chars;
};
int main(int argc, char **argv) {
    if (argc != 3) {
        return 1;
    }
    string server_name(argv[1]);
    uint16_t port = static_cast<uint16_t>(stoi(string(argv[2])));
    string ip("127.0.0.1");
    ServerOption server_option;
    shared_ptr<Service> service = make_shared<Chargen>();
    shared_ptr<Server> server =
        get_server(service, server_name, ip, port, server_option);
    server->run();
    return 0;
}
