#include "net/util/error.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
void syscall_error() {
    char *errorStr = strerror(errno);
    printf("Syscall error: %s\n", errorStr);
    exit(1);
}
void fatal_error(const char *s) {
    printf("Fatal error: %s\n", s);
    exit(1);
}
