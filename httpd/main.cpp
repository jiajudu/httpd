#include "server/forkServer.h"
#include "server/iterativeServer.h"
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
    shared_ptr<Server> server = make_shared<ThreadedServer>(ip, 1234);
    Echo echo(server);
    server->run();
    return 0;
}
