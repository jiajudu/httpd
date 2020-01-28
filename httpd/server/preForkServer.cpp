#include "server/preForkServer.h"
#include "auxiliary/error.h"
#include <algorithm>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
using std::bind;
using std::make_shared;
using std::max;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
ChildProcess::ChildProcess(int _fd) : fd(_fd), busy(false) {
}
PreForkServer::PreForkServer(string &_ip, uint16_t _port, int _numProcess)
    : ip(_ip), port(_port), numProcess(_numProcess) {
}
void PreForkServer::run() {
    if (numProcess <= 0) {
        exit(1);
    }
    for (int i = 0; i < numProcess; i++) {
        int fds[2];
        int ret = socketpair(Socket::domainLocal, SOCK_STREAM, 0, fds);
        if (ret < 0) {
            fatalError();
        }
        int pid = fork();
        if (pid < 0) {
            fatalError();
        }
        if (pid > 0) {
            close(fds[1]);
            childs.push_back(ChildProcess(fds[0]));
        } else {
            for (int j = 0; j < i; j++) {
                close(childs[j].fd);
            }
            close(fds[0]);
            childMain(fds[1]);
        }
    }
    listenSocket = make_shared<Socket>(Socket::domainINET, false, false);
    listenSocket->bind(ip, port);
    listenSocket->listen(10);
    while (true) {
        shared_ptr<Socket> conn = listenSocket->accept();
        size_t childIndex = getAvailableProcess();
        printf("sendconn %d\n", conn->getFd());
        sendConn(childs[childIndex].fd, conn);
        childs[childIndex].busy = true;
        conn->close();
    }
}
size_t PreForkServer::getAvailableProcess() {
    for (size_t i = 0; i < childs.size(); i++) {
        if (!childs[i].busy) {
            return i;
        }
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    int nfds = 0;
    for (size_t i = 0; i < childs.size(); i++) {
        FD_SET(childs[i].fd, &readfds);
        nfds = max(nfds, childs[i].fd);
    }
    int ret = select(nfds + 1, &readfds, 0, 0, 0);
    if (ret < 0) {
        fatalError();
    }
    for (size_t i = 0; i < childs.size(); i++) {
        if (FD_ISSET(childs[i].fd, &readfds)) {
            char buf[1];
            read(childs[i].fd, buf, 1);
            childs[i].busy = false;
        }
    }
    for (size_t i = 0; i < childs.size(); i++) {
        if (!childs[i].busy) {
            return i;
        }
    }
    exit(1);
}
void PreForkServer::childMain(int fd) {
    while (true) {
        shared_ptr<Socket> conn = recvConn(fd);
        printf("recvconn %d\n", conn->getFd());
        char buf[4096];
        ssize_t size = conn->recv(buf, 4096);
        while (size > 0) {
            onMessage(buf, size, bind(&Socket::_send, conn, _1, _2));
            size = conn->recv(buf, 4096);
        }
        conn->close();
        write(fd, buf, 1);
    }
}
void PreForkServer::sendConn(int fd, shared_ptr<Socket> conn) {
    struct iovec iov[1];
    struct msghdr msg;
    char buf[24];
    int connfd = conn->getFd();
    int domain = conn->getDomain();
    bool nonBlock = conn->getNonBlock();
    bool closeExec = conn->getCloseExec();
    memcpy(buf, &domain, sizeof(int));
    memcpy(buf + 8, &nonBlock, sizeof(bool));
    memcpy(buf + 16, &closeExec, sizeof(bool));
    iov[0].iov_base = buf;
    iov[0].iov_len = 24;
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
        fatalError();
    }
}
shared_ptr<Socket> PreForkServer::recvConn(int fd) {
    struct iovec iov[1];
    struct msghdr msg;
    char buf[24];
    iov[0].iov_base = buf;
    iov[0].iov_len = 24;
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
    int nonBlock = *(reinterpret_cast<bool *>(buf + 8));
    int closeExec = *(reinterpret_cast<bool *>(buf + 16));
    return make_shared<Socket>(connfd, domain, nonBlock, closeExec);
}