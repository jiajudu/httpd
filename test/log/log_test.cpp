#include "net/logging/logger.h"
#include "net/logging/loggingstream.h"
void thread_func(shared_ptr<Logger> logger) {
    string a(100, 'a');
    for (int i = 0; i < 1e6; i++) {
        LOG_INFO << a;
    }
}
int main() {
    shared_ptr<Logger> logger = make_shared<Logger>("../build/log");
    vector<thread> ts;
    for (int i = 0; i < 4; i++) {
        ts.push_back(thread(thread_func, logger));
    }
    for (size_t i = 0; i < 4; i++) {
        ts[i].join();
    }
    return 0;
}