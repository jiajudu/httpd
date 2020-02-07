#include "service/service.h"
class Timestamp : public Service {
public:
    void onConnection(shared_ptr<Connection> conn);
};
