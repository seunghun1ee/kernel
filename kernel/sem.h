#ifndef __SEM_H
#define __SEM_H



#include <stdbool.h>

typedef struct {
    int value;
    bool init;
} sem_t;



extern int sem_init(sem_t *sem, int value);
extern int sem_post(sem_t *sem);
extern int sem_wait(sem_t *sem);
extern int sem_destroy(sem_t *sem);
extern int pipe(int *fd[2]);
extern int close(int fd);

#endif