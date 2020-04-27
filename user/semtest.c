#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc.h"

sem_t test_lock;
int s;

void main_semtest() {

    sem_init(&test_lock, 1);
    int pid = fork();

    
        if(pid == 0) {
            int err = sem_wait(&test_lock);
            if(err != 0) {
                write(STDOUT_FILENO, "error sem is closed", 20);
                exit(EXIT_SUCCESS);
                
            }
            s = 0;
            write(STDOUT_FILENO, "child", 6);
            sem_post(&test_lock);
            sem_destroy(&test_lock);
        }
        else {
            int err = sem_wait(&test_lock);
            if(err != 0) {
                write(STDOUT_FILENO, "error sem is closed", 20);
                exit(EXIT_SUCCESS);
                
            }
            s = 1;
            write(STDOUT_FILENO, "parent", 7);
            sem_post(&test_lock);
            sem_destroy(&test_lock);
        }
        
    

    

    exit(EXIT_SUCCESS);
}