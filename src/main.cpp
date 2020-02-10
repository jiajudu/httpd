#include "http/config.h"
#include "http/http.h"
#include "net/server/server.h"
#include <iostream>
#include <memory>
#include <string>
int main(int argc, char **argv) {
    if (argc != 2) {
        return 1;
    }
    HTTPDConfig config = parse_http_config(argv[1]);
    uint16_t port = config.port;
    ServerOption server_option;
    if (config.concurrence == "threadPoolReactor") {
        server_option.thread_number = config.concurrence_number;
    }
    if (config.concurrence == "processPoolReactor") {
        server_option.process_number = config.concurrence_number;
    }
    server_option.timeout = config.timeout;
    shared_ptr<Service> service = make_shared<HTTP>(config);
    shared_ptr<Server> server =
        get_server(service, config.concurrence, "0.0.0.0", port, server_option);
    server->run();
    return 0;
}
