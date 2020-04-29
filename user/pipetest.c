#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc.h"
#include "IPC.h"

int pipefd[2];
int failfd[2];

void main_pipetest() {
    failfd[0] = 3;
    pipe(&pipefd);

    exit(EXIT_SUCCESS);
}