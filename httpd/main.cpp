#include "server/forkServer.h"
#include "server/iterativeServer.h"
#include "server/preForkServer.h"
#include "server/threadedServer.h"
#include "service/echo.h"
#include <iostream>
#include <memory>
#include <string>
using std::bind;
using std::function;
using namespace std::placeholders;
using std::make_shared;
using std::static_pointer_cast;
int main() {
    string ip("127.0.0.1");
    shared_ptr<Server> server = make_shared<PreForkServer>(ip, 1234, 10);
    Echo echo(server);
    server->run();
    return 0;
}
