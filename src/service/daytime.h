#include "service/service.h"
class DayTime : public Service {
public:
    void onConnection(shared_ptr<Connection> conn);
};
