#include "service/discard.h"
#include <functional>
Discard::Discard() {
    decoder = bind(&Discard::decode, this, _1, _2);
}
size_t Discard::decode(char *s, size_t n) {
    (void)s;
    return n;
}
