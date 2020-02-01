#include "service/service.h"
class Timestamp : public Service {
public:
    Timestamp();
    void onConnection(shared_ptr<Connection> conn);

private:
    size_t decode(char *s, size_t n);
};
