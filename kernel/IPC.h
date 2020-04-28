#ifndef __IPC_H
#define __IPC_H

#include <stdbool.h>

typedef struct {
    int value;
    bool init;
} sem_t;

extern int sem_init(sem_t *sem, int value);
extern int sem_post(sem_t *sem);
extern int sem_wait(sem_t *sem);
extern int sem_destroy(sem_t *sem);

#endif