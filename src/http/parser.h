#pragma once
#include "net/socket/connection.h"
#include <string>
#include <unistd.h>
size_t http_decoder(char *s_buf, size_t n_buf, size_t max_len,
                    size_t expected_length);
int parse_header(shared_ptr<Connection> conn, string &s);