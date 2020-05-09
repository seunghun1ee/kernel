#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc.h"

void main_displaytest() {
    write(STDOUT_FILENO, "TEST\n", 5);
    for(unsigned char i = 32; i < 255; i++) {
        char test[1];
        test[0] = i;
        write(STDOUT_FILENO, test, 1);
        write(STDOUT_FILENO, " ", 1);
    }
    exit(EXIT_SUCCESS);
}