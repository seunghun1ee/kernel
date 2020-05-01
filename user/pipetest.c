#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc.h"
#include "IPC.h"

int pipefd[2];

void main_pipetest() {

    char b[10];
    char c[6];

    int pipe_err = pipe(&pipefd);
    if(pipe_err != 0) {
        write(STDOUT_FILENO, "pipe init error ", 17);
        exit(EXIT_SUCCESS);
    }

    write(pipefd[1], "abc", 4);
    read(pipefd[0], b, 2);
    read(pipefd[0], c, 2);

    write(STDOUT_FILENO, b, 2);
    write(STDOUT_FILENO, " ", 2);
    write(STDOUT_FILENO, c, 2);

    close(pipefd[0]);
    close(pipefd[1]);

    exit(EXIT_SUCCESS);
}