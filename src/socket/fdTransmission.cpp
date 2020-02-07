#include "socket/fdTransmission.h"
#include "auxiliary/error.h"
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
FDTransmission::FDTransmission() {
    int fds[2];
    int ret = socketpair(Socket::domain_local, SOCK_STREAM, 0, fds);
    if (ret < 0) {
        syscall_error();
    }
    fd_parent = fds[0];
    fd_child = fds[1];
    fd = -1;
}
void FDTransmission::parent() {
    close(fd_child);
    fd = fd_parent;
}
void FDTransmission::child() {
    close(fd_parent);
    fd = fd_child;
}
int FDTransmission::get_fd() const {
    return fd;
}
void FDTransmission::send_conn(shared_ptr<Connection> conn) {
    if (!conn->can_be_sent()) {
        fatal_error("this connection cannot be sent");
    }
    struct iovec iov[1];
    struct msghdr msg;
    char buf[8];
    int connfd = conn->get_socket()->get_fd();
    int domain = conn->get_socket()->get_domain();
    memcpy(buf, &domain, sizeof(int));
    iov[0].iov_base = buf;
    iov[0].iov_len = 16;
    msg.msg_name = 0;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    cmsghdr cm;
    cm.cmsg_len = CMSG_LEN(sizeof(int));
    cm.cmsg_level = SOL_SOCKET;
    cm.cmsg_type = SCM_RIGHTS;
    *(int *)CMSG_DATA(&cm) = connfd;
    msg.msg_control = &cm;
    msg.msg_controllen = CMSG_LEN(sizeof(int));
    ssize_t ret = sendmsg(fd, &msg, 0);
    if (ret < 0) {
        syscall_error();
    }
}
shared_ptr<Connection> FDTransmission::recv_conn() {
    struct iovec iov[1];
    struct msghdr msg;
    char buf[8];
    iov[0].iov_base = buf;
    iov[0].iov_len = 16;
    msg.msg_name = 0;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    struct cmsghdr cm;
    msg.msg_control = &cm;
    msg.msg_controllen = CMSG_LEN(sizeof(int));
    recvmsg(fd, &msg, 0);
    int connfd = *(int *)CMSG_DATA(&cm);
    int domain = *(reinterpret_cast<int *>(buf));
    return make_shared<Connection>(make_shared<Socket>(connfd, domain));
}
