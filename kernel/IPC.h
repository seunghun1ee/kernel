#ifndef __IPC_H
#define __IPC_H

#define SYS_PIPE     ( 0x08 )
#define SYS_CLOSE    ( 0x09 )

#define MAX_PIPES 20

#include <stdbool.h>

typedef struct {
    int value;
    bool init;
} sem_t;

typedef struct {
    int read_end;
    int write_end;
    char *queue;
    bool taken;
} pipe_t;

typedef struct {
    int pipeIndex;
    bool taken;
} file_descriptor_t;

extern int sem_init(sem_t *sem, int value);
extern int sem_post(sem_t *sem);
extern int sem_wait(sem_t *sem);
extern int sem_destroy(sem_t *sem);
extern int pipe(int *fd[2]);
extern int close(int fd);

#endif