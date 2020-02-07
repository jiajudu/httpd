#include "net/util/buffer.h"
Buffer::Buffer() : buf(1024), write_ptr(0), read_ptr(0) {
}
void Buffer::prepare_space(size_t n) {
    if (n + write_ptr > buf.size()) {
        if (n + write_ptr - read_ptr <= buf.size()) {
            copy(&buf[read_ptr], &buf[write_ptr], &buf[0]);
        } else {
            vector<char> t(max(n + write_ptr - read_ptr, buf.size() * 2));
            copy(&buf[read_ptr], &buf[write_ptr], &t[0]);
            swap(t, buf);
        }
        write_ptr = write_ptr - read_ptr;
        read_ptr = 0;
    }
}
size_t Buffer::write(const char *s, size_t n) {
    prepare_space(n);
    copy(s, s + n, &buf[write_ptr]);
    write_ptr += n;
    return n;
}
size_t
Buffer::read(function<size_t(char *s_buf, size_t n_buf)> decode_and_copy) {
    size_t n = decode_and_copy(&buf[read_ptr], min(n, size()));
    read_ptr += n;
    return n;
}
size_t Buffer::size() const {
    return write_ptr - read_ptr;
}