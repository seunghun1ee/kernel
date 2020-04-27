#ifndef __IPC_H
#define __IPC_H

typedef int sem_t;
extern void sem_init(sem_t *sem, int value);
extern int sem_post(sem_t *sem);
extern int sem_wait(sem_t *sem);
extern void sem_destroy(sem_t *sem);

#endif