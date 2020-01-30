#include "auxiliary/ip.h"
#include <arpa/inet.h>
uint32_t inet_ston(string &s) {
    uint32_t addr = 0;
    inet_pton(AF_INET, s.c_str(), &addr);
    return addr;
}
