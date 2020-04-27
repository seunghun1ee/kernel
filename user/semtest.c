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
            int wait_err = sem_wait(&test_lock);
            if(wait_err != 0) {
                write(STDOUT_FILENO, "wait error, sem is closed", 26);
                exit(EXIT_SUCCESS);
                
            }
            s = 0;
            write(STDOUT_FILENO, "child", 6);
            int post_err = sem_post(&test_lock);
            if(post_err != 0) {
                write(STDOUT_FILENO, "post error, sem is closed", 26);
                exit(EXIT_SUCCESS);
            }
            //sem_destroy(&test_lock);
        }
        else {
            int wait_err = sem_wait(&test_lock);
            if(wait_err != 0) {
                write(STDOUT_FILENO, "wait error, sem is closed", 26);
                exit(EXIT_SUCCESS);
                
            }
            s = 1;
            write(STDOUT_FILENO, "parent", 7);
            sem_destroy(&test_lock);
            int post_err = sem_post(&test_lock);
            if(post_err != 0) {
                write(STDOUT_FILENO, "post error, sem is closed", 26);
                exit(EXIT_SUCCESS);
            }
            //sem_destroy(&test_lock);
        }
        
    

    

    exit(EXIT_SUCCESS);
}