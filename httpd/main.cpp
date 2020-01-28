#include "server/forkServer.h"
#include "server/iterativeServer.h"
#include "server/preForkServer.h"
#include "server/preThreadedServer.h"
#include "server/threadedServer.h"
#include "service/echo.h"
#include <iostream>
#include <memory>
#include <string>
int main() {
    string ip("127.0.0.1");
    shared_ptr<Server> server = make_shared<PreThreadedServer>(ip, 1234, 10);
    Echo echo(server);
    server->run();
    return 0;
}
