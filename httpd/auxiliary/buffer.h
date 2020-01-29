#pragma once
#include "auxiliary/std.h"
#include <vector>
class Buffer {
public:
    Buffer();
    size_t size() const;
    void write(vector<char> &s);
    vector<char> read(size_t n = 0);

private:
    vector<char> buf;
    size_t writePtr;
    size_t readPtr;
};