#include "server/server.h"
#include <iostream>
#include <memory>
#include <string>
int main() {
    std::string ip("127.0.0.1");
    std::shared_ptr<Server> server = std::make_shared<Server>(ip, 1234);
    server->run();
    return 0;
}
