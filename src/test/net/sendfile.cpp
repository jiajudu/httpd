#include "net/server/server.h"
#include "net/service/service.h"
#include "net/socket/socket.h"
#include "net/util/noncopyable.h"
#include <functional>
#include <linux/unistd.h>
#include <memory>
#include <stdlib.h>
class Sendfile : public Service {
public:
    void onConnection(shared_ptr<Connection> conn) {
        FILE *f = fopen("../../../project/gfwlist/gfwlist.txt", "r");
        conn->data = shared_ptr<FILE>(f, [](FILE *fp) -> void { fclose(fp); });
        char buf[4096];
        size_t ret = fread(buf, 1, 4096, f);
        if (ret == 0) {
            conn->close();
        } else {
            conn->send(string(buf, buf + ret));
        }
    }
    void onSendComplete(shared_ptr<Connection> conn) {
        FILE *f = any_cast<shared_ptr<FILE>>(conn->data).get();
        char buf[4096];
        size_t ret = fread(buf, 1, 4096, f);
        if (ret == 0) {
            conn->close();
        } else {
            conn->send(string(buf, buf + ret));
        }
    }
};
int main(int argc, char **argv) {
    if (argc != 3) {
        return 1;
    }
    string server_name(argv[1]);
    uint16_t port = static_cast<uint16_t>(stoi(string(argv[2])));
    string ip("127.0.0.1");
    ServerOption server_option;
    shared_ptr<Service> service = make_shared<Sendfile>();
    shared_ptr<Server> server =
        get_server(service, server_name, ip, port, server_option);
    server->run();
    return 0;
}