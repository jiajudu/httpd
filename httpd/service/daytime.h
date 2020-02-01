#include "service/service.h"
class DayTime : public Service {
public:
    DayTime();
    void onConnection(shared_ptr<Connection> conn);

private:
    size_t decode(char *s, size_t n);
};
