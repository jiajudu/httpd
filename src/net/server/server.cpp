#include "net/server/processPoolReactorServer.h"
#include "net/server/reactorServer.h"
#include "net/server/threadPoolReactorServer.h"
shared_ptr<Server> get_server(shared_ptr<Service> service,
                              const string &server_name, string ip,
                              uint16_t port, ServerOption &option) {
    if (server_name == "processPollReactor") {
        return make_shared<ProcessPoolReactorServer>(service, ip, port, option);
    } else if (server_name == "reactor") {
        return make_shared<ReactorServer>(service, ip, port, option);
    } else if (server_name == "threadPoolReactor") {
        return make_shared<ThreadPoolReactorServer>(service, ip, port, option);
    } else {
        exit(1);
    }
}