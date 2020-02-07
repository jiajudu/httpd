#pragma once
void syscall_error() __attribute__((noreturn));
void fatal_error(const char *s) __attribute__((noreturn));
