#include "server/processPoolReactorServer.h"
#include "server/reactorServer.h"
#include "server/threadPoolReactorServer.h"
#include "service/chargen.h"
#include "service/chat.h"
#include "service/daytime.h"
#include "service/discard.h"
#include "service/echo.h"
#include "service/http.h"
#include "service/limitedEcho.h"
#include "service/sendfile.h"
#include "service/timestamp.h"
#include <iostream>
#include <memory>
#include <string>
shared_ptr<Service> get_service(string &service_name) {
    if (service_name == "echo") {
        return make_shared<Echo>();
    } else if (service_name == "discard") {
        return make_shared<Discard>();
    } else if (service_name == "daytime") {
        return make_shared<DayTime>();
    } else if (service_name == "timestamp") {
        return make_shared<Timestamp>();
    } else if (service_name == "chargen") {
        return make_shared<Chargen>();
    } else if (service_name == "sendfile") {
        return make_shared<Sendfile>();
    } else if (service_name == "chat") {
        return make_shared<Chat>();
    } else if (service_name == "limitedecho") {
        return make_shared<LimitedEcho>();
    } else if (service_name == "http") {
        return make_shared<HTTP>();
    } else {
        exit(1);
    }
}
shared_ptr<Server> get_server(shared_ptr<Service> service, string &server_name,
                              string ip, uint16_t port) {
    ServerOption server_option;
    server_option.process_number = 4;
    server_option.thread_number = 4;
    server_option.max_connection_number = 4;
    if (server_name == "processpollreactor") {
        return make_shared<ProcessPoolReactorServer>(service, ip, port,
                                                     server_option);
    } else if (server_name == "reactor") {
        return make_shared<ReactorServer>(service, ip, port, server_option);
    } else if (server_name == "threadpoolreactor") {
        return make_shared<ThreadPoolReactorServer>(service, ip, port,
                                                    server_option);
    } else {
        exit(1);
    }
}
int main(int argc, char **argv) {
    if (argc != 4) {
        return 1;
    }
    string service_name(argv[1]);
    string server_name(argv[2]);
    uint16_t port = static_cast<uint16_t>(stoi(string(argv[3])));
    string ip("127.0.0.1");
    shared_ptr<Service> service = get_service(service_name);
    shared_ptr<Server> server = get_server(service, server_name, ip, port);
    server->run();
    return 0;
}
