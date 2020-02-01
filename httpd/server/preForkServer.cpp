#include "server/preForkServer.h"
#include "auxiliary/error.h"
#include <algorithm>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
PreForkServer::PreForkServer(shared_ptr<Service> _service, string &_ip,
                             uint16_t _port, int _numProcess)
    : Server(_service, _ip, _port), numProcess(_numProcess) {
}
void PreForkServer::run() {
    if (numProcess <= 0) {
        exit(1);
    }
    for (int i = 0; i < numProcess; i++) {
        FDTransmission fdt;
        int pid = fork();
        if (pid < 0) {
            syscall_error();
        }
        if (pid > 0) {
            fdt.parent();
            childs.push_back(pair<FDTransmission, bool>(fdt, false));
        } else {
            for (int j = 0; j < i; j++) {
                fdt.child();
            }
            fdt.child();
            child_main(fdt);
        }
    }
    listener = make_shared<Listener>(ip, port, 10);
    while (true) {
        shared_ptr<Connection> conn = listener->accept();
        size_t child_index = getAvailableProcess();
        childs[child_index].first.send_conn(conn);
        childs[child_index].second = true;
        conn->close();
    }
}
size_t PreForkServer::getAvailableProcess() {
    for (size_t i = 0; i < childs.size(); i++) {
        if (!childs[i].second) {
            return i;
        }
    }
    fd_set readfds;
    FD_ZERO(&readfds);
    int nfds = 0;
    for (size_t i = 0; i < childs.size(); i++) {
        FD_SET(childs[i].first.get_fd(), &readfds);
        nfds = max(nfds, childs[i].first.get_fd());
    }
    int ret = select(nfds + 1, &readfds, 0, 0, 0);
    if (ret < 0) {
        syscall_error();
    }
    for (size_t i = 0; i < childs.size(); i++) {
        if (FD_ISSET(childs[i].first.get_fd(), &readfds)) {
            char buf[1];
            read(childs[i].first.get_fd(), buf, 1);
            childs[i].second = false;
        }
    }
    for (size_t i = 0; i < childs.size(); i++) {
        if (!childs[i].second) {
            return i;
        }
    }
    exit(1);
}
void PreForkServer::child_main(FDTransmission &fdt) {
    while (true) {
        shared_ptr<Connection> conn = fdt.recv_conn();
        service->onConnection(conn);
        string buf(4096, 0);
        size_t size = 4096;
        while (conn->active()) {
            if (size > 0) {
                size = conn->recv(buf);
                string message(buf.begin(), buf.begin() + size);
                service->onMessage(conn, message);
            } else {
                conn->close();
            }
        }
        write(fdt.get_fd(), &buf[0], 1);
    }
}
