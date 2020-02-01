#include "service/daytime.h"
#include "auxiliary/tm.h"
DayTime::DayTime() {
}
void DayTime::onConnection(shared_ptr<Connection> conn) {
    string t = get_time_fmt();
    conn->send(t);
    conn->close();
}
void DayTime::onMessage(shared_ptr<Connection> conn, string &input_message) {
    (void)conn;
    (void)input_message;
}
size_t DayTime::decode(char *s, size_t n) {
    (void)s;
    return n;
}
