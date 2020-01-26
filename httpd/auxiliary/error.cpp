#include "auxiliary/error.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
void fatalError() {
    char *errorStr = strerror(errno);
    write(1, errorStr, strlen(errorStr));
    write(1, "\n", 1);
    exit(1);
}
