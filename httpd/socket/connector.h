#include "socket/connection.h"
class Connector {
public:
    static shared_ptr<Connection> connect(string &ip, uint16_t port);
};
