#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc.h"

int pipefd[2];

void main_pipetest() {

    char *input = "hello";
    write(STDOUT_FILENO, "Test input: ", 12);
    read(STDIN_FILENO, input, 5);
    write(STDOUT_FILENO, "\n", 1);
    write(STDOUT_FILENO, input, 5);
    write(STDERR_FILENO, "error? ", 8);

    int failfd = 7;
    char b[10];
    char c[6];

    int pipe_err = pipe(pipefd);
    if(pipe_err != 0) {
        write(STDOUT_FILENO, "pipe init error ", 17);
        exit(EXIT_SUCCESS);
    }

    int pid = fork();

    if(pid == 0) {
        //child
        read(pipefd[0], c, 8);
        write(STDOUT_FILENO, c, 2);
        read(pipefd[0], c, 8);

        close(pipefd[0]);
        close(pipefd[1]);
    }
    else {
        //parent
        write(pipefd[1], "abcasdfgwraasdf", 16);
        read(pipefd[0], b, 2);
        write(STDOUT_FILENO, b, 2);
    }

    
    

    
    // if(close(failfd) < 0) {
    //     write(STDOUT_FILENO, "close error, not opend before ", 31);
    // }

    exit(EXIT_SUCCESS);
}