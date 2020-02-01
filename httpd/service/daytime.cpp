#include "service/daytime.h"
#include "auxiliary/tm.h"
DayTime::DayTime() {
}
void DayTime::onConnection(shared_ptr<Connection> conn) {
    string t = get_time_fmt();
    conn->send(t);
    conn->close();
}
size_t DayTime::decode(char *s, size_t n) {
    (void)s;
    return n;
}
