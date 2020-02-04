#include "fastcgi/fastcgi.h"
void FastCGITask::add_env(const string &key, const string &value) {
    size_t len = key.size() + value.size() + 2;
    if (key.size() >= 128) {
        len += 3;
    }
    if (value.size() >= 128) {
        len += 3;
    }
    string r(len, 0);
    int ptr = 0;
    if (key.size() < 128) {
        size_t size = key.size();
        r[ptr] = static_cast<char>(size);
        ptr++;
    } else {
        size_t size = key.size();
        r[ptr] = static_cast<char>((size >> 24) | 0x80);
        r[ptr + 1] = static_cast<char>((size >> 16) & 0xff);
        r[ptr + 2] = static_cast<char>((size >> 8) & 0xff);
        r[ptr + 3] = static_cast<char>(size & 0xff);
        ptr += 4;
    }
    if (value.size() < 128) {
        size_t size = value.size();
        r[ptr] = static_cast<char>(size);
        ptr++;
    } else {
        size_t size = value.size();
        r[ptr] = static_cast<char>((size >> 24) | 0x80);
        r[ptr + 1] = static_cast<char>((size >> 16) & 0xff);
        r[ptr + 2] = static_cast<char>((size >> 8) & 0xff);
        r[ptr + 3] = static_cast<char>(size & 0xff);
        ptr += 4;
    }
    string::iterator it = r.begin() + ptr;
    it = copy(key.begin(), key.end(), it);
    it = copy(value.begin(), value.end(), it);
    if (envs.size() == 0 || envs.back().size() + r.size() > 32768) {
        envs.push_back(r);
    } else {
        envs.back() += r;
    }
}
void FastCGITask::add_content(const string &c) {
    stdins.push_back(c);
}
FastCGI::FastCGI(string &sock) {
    if (sock.compare(0, 5, "unix:") == 0) {
        string f = sock.substr(5);
    }
}