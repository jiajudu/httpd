#pragma once
#include "net/socket/connection.h"
void http_error(shared_ptr<Connection> conn, int error_code);