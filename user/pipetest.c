#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc.h"
#include "IPC.h"

int pipefd[2];

void main_pipetest() {
    int pipe_err = pipe(&pipefd);
    if(pipe_err != 0) {
        write(STDOUT_FILENO, "pipe init error ", 17);
        exit(EXIT_SUCCESS);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    exit(EXIT_SUCCESS);
}