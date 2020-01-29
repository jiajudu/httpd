#include "auxiliary/buffer.h"
Buffer::Buffer() : buf(1024), writePtr(0), readPtr(0) {
}
size_t Buffer::size() const {
    return writePtr - readPtr;
}
void Buffer::write(vector<char> &s) {
    if (s.size() + writePtr <= buf.size()) {
        copy(s.begin(), s.end(), &s[writePtr]);
        writePtr += s.size();
    } else if (s.size() + writePtr - readPtr <= buf.size()) {
        copy(&buf[readPtr], &buf[writePtr], &buf[0]);
        writePtr = writePtr - readPtr;
        readPtr = 0;
        copy(s.begin(), s.end(), &s[writePtr]);
        writePtr += s.size();
    } else {
        vector<char> t(max(s.size() + writePtr - readPtr, buf.size() * 2));
        copy(&buf[readPtr], &buf[writePtr], &t[0]);
        copy(s.begin(), s.end(), &t[writePtr - readPtr]);
        writePtr = writePtr - readPtr + s.size();
        readPtr = 0;
        swap(t, buf);
    }
}
vector<char> Buffer::read(size_t n) {
    if (n == 0 || writePtr - readPtr < n) {
        n = writePtr - readPtr;
    }
    vector<char> ret(n);
    copy(&buf[readPtr], &buf[readPtr + n], ret.begin());
    readPtr += n;
    return ret;
}