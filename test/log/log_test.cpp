#include "net/logging/logger.h"
#include "net/logging/loggingstream.h"
#include "net/util/tm.h"
#include <iostream>
void thread_func(shared_ptr<Logger> logger) {
    string a(67, 'a');
    for (int i = 0; i < 25000000; i++) {
        LOG_INFO << a << '\n';
    }
}
int main() {
    cout << get_time_fmt() << endl;
    {
        shared_ptr<Logger> logger = make_shared<Logger>("build/log");
        vector<thread> ts;
        for (int i = 0; i < 4; i++) {
            ts.push_back(thread(thread_func, logger));
        }
        for (size_t i = 0; i < 4; i++) {
            ts[i].join();
        }
    }
    cout << get_time_fmt() << endl;
    return 0;
}