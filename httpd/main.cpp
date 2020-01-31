#include "server/forkServer.h"
#include "server/iterativeServer.h"
#include "server/preForkServer.h"
#include "server/preThreadedServer.h"
#include "server/processPoolReactorServer.h"
#include "server/reactorServer.h"
#include "server/threadPoolReactorServer.h"
#include "server/threadedServer.h"
#include "service/echo.h"
#include <iostream>
#include <memory>
#include <string>
int main(int argc, char **argv) {
    if (argc != 2) {
        return 1;
    }
    uint16_t port = static_cast<uint16_t>(stoi(string(argv[1])));
    string ip("127.0.0.1");
    shared_ptr<Server> server =
        make_shared<ProcessPoolReactorServer>(ip, port, 2);
    Echo echo(server);
    server->run();
    return 0;
}
