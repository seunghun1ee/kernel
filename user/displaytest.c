#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc.h"

void main_displaytest() {
    write(STDOUT_FILENO, "TEST", 4);
    exit(EXIT_SUCCESS);
}