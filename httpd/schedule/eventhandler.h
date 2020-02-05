#pragma once
#include "auxiliary/std.h"
#include <functional>
class EventHandler {
public:
    function<void(int fd)> read = 0;
    function<void(int fd)> write = 0;
    function<void(int fd)> hang_up = 0;
    function<void(int fd)> error = 0;
};