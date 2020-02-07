#pragma once
#include "auxiliary/std.h"
#include <condition_variable>
#include <mutex>
#include <queue>
template <class T> class BlockingQueue {
public:
    BlockingQueue(typename queue<T>::size_type n) : capacity(n) {
    }
    typename queue<T>::size_type capacity;
    mutex lock;
    condition_variable condition_push;
    condition_variable condition_pop;
    queue<T> q;
    void push(const T &value) {
        unique_lock<mutex> ulock(lock);
        while (q.size() >= capacity) {
            condition_push.wait(ulock);
        }
        q.push(value);
        condition_pop.notify_one();
    }
    T pop() {
        unique_lock<mutex> ulock(lock);
        while (q.size() <= 0) {
            condition_pop.wait(ulock);
        }
        auto ret = q.front();
        q.pop();
        return ret;
    }
};
template <class T> class Queue {
public:
    Queue() {
    }
    mutex lock;
    condition_variable condition_push;
    condition_variable condition_pop;
    queue<T> q;
    void push(const T &value) {
        unique_lock<mutex> ulock(lock);
        q.push(value);
        condition_pop.notify_one();
    }
    T pop() {
        unique_lock<mutex> ulock(lock);
        while (q.size() <= 0) {
            condition_pop.wait(ulock);
        }
        auto ret = q.front();
        q.pop();
        return ret;
    }
    void pop_all(queue<T> &s) {
        unique_lock<mutex> ulock(lock);
        swap(s, q);
    }
};
