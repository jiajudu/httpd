#include "auxiliary/ip.h"
#include <arpa/inet.h>
uint32_t inet_ston(string &s) {
    uint32_t addr = 0;
    inet_pton(AF_INET, s.c_str(), &addr);
    return addr;
}
string inet_ntos(uint32_t addr) {
    uint32_t a = ntohl(addr);
    return to_string(a >> 24) + '.' + to_string((a >> 16) & 0xff) + '.' +
           to_string((a >> 8) && 0xff) + '.' + to_string(a & 0xff);
}