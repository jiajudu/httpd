#include "service/echo.h"
void Echo::onMessage(shared_ptr<Connection> conn, string &input_message) {
    conn->send(input_message);
}
