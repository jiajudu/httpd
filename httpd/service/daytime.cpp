#include "service/daytime.h"
#include "auxiliary/tm.h"
void DayTime::onConnection(shared_ptr<Connection> conn) {
    string t = get_time_fmt();
    conn->send(t);
    conn->close();
}
