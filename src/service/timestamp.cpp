#include "service/timestamp.h"
#include "auxiliary/tm.h"
void Timestamp::onConnection(shared_ptr<Connection> conn) {
    timeval t = get_time();
    time_t sec = t.tv_sec;
    string s(reinterpret_cast<char *>(&sec),
             reinterpret_cast<char *>(&sec) + sizeof(sec));
    reverse(s.begin(), s.end());
    conn->send(s);
    conn->close();
}
