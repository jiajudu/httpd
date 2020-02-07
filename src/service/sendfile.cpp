#include "service/sendfile.h"
#include <linux/unistd.h>
#include <memory>
#include <stdlib.h>
void Sendfile::onConnection(shared_ptr<Connection> conn) {
    FILE *f = fopen("../../../project/gfwlist/gfwlist.txt", "r");
    conn->data = shared_ptr<FILE>(f, [](FILE *fp) -> void { fclose(fp); });
    char buf[4096];
    size_t ret = fread(buf, 1, 4096, f);
    if (ret == 0) {
        conn->close();
    } else {
        conn->send(string(buf, buf + ret));
    }
}
void Sendfile::onSendComplete(shared_ptr<Connection> conn) {
    FILE *f = any_cast<shared_ptr<FILE>>(conn->data).get();
    char buf[4096];
    size_t ret = fread(buf, 1, 4096, f);
    if (ret == 0) {
        conn->close();
    } else {
        conn->send(string(buf, buf + ret));
    }
}
