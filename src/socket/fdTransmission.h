#pragma once
#include "socket/connection.h"
class FDTransmission {
public:
    FDTransmission();
    void send_conn(shared_ptr<Connection> conn);
    shared_ptr<Connection> recv_conn();
    void parent();
    void child();
    int get_fd() const;

private:
    int fd_parent;
    int fd_child;
    int fd;
};