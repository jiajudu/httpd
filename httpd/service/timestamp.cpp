#include "service/timestamp.h"
#include "auxiliary/tm.h"
Timestamp::Timestamp() {
}
void Timestamp::onConnection(shared_ptr<Connection> conn) {
    timeval t = get_time();
    time_t sec = t.tv_sec;
    string s(reinterpret_cast<char *>(&sec),
             reinterpret_cast<char *>(&sec) + sizeof(sec));
    reverse(s.begin(), s.end());
    conn->send(s);
    conn->close();
}
size_t Timestamp::decode(char *s, size_t n) {
    (void)s;
    return n;
}
