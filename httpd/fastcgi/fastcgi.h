#pragma once
#include "auxiliary/std.h"
#include <vector>
class FastCGITask {
public:
    void add_env(const string &key, const string &value);
    void add_content(const string &c);

private:
    vector<string> envs;
    vector<string> stdins;
};
class FastCGI {
public:
    FastCGI(string &sock);
};