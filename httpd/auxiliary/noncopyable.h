#pragma once
class Noncopyable {
public:
    Noncopyable() {
    }
    Noncopyable(const Noncopyable &) = delete;
    Noncopyable &operator=(const Noncopyable &) = delete;
};
