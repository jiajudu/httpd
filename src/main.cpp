#include "http/http.h"
#include "net/server/server.h"
#include <iostream>
#include <memory>
#include <string>
int main(int argc, char **argv) {
    if (argc != 3) {
        return 1;
    }
    string server_name(argv[1]);
    uint16_t port = static_cast<uint16_t>(stoi(string(argv[2])));
    string ip("127.0.0.1");
    ServerOption server_option;
    shared_ptr<Service> service = make_shared<HTTP>();
    shared_ptr<Server> server =
        get_server(service, server_name, ip, port, server_option);
    server->run();
    return 0;
}
