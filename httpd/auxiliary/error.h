#pragma once
void syscall_error() __attribute__((noreturn));
void agreement_error(const char *s) __attribute__((noreturn));
