#include "http/error.h"
void http_error(shared_ptr<Connection> conn, int error_code) {
    (void)conn;
    (void)error_code;
}
