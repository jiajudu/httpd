#pragma once
#include "net/util/std.h"
#include <functional>
class EventHandler {
public:
    function<void(int fd)> read = 0;
    function<void(int fd)> write = 0;
    function<void(int fd)> close = 0;
    function<void(int fd)> error = 0;
};