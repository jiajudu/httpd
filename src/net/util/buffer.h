#pragma once
#include "net/util/std.h"
#include <vector>
class Buffer {
public:
    Buffer();
    size_t write(const char *s, size_t n);
    size_t read(function<size_t(char *s, size_t n)> decode_and_copy);
    size_t size() const;
    void swap_buf(Buffer &b);

private:
    void prepare_space(size_t n);
    vector<char> buf;
    size_t write_ptr;
    size_t read_ptr;
};