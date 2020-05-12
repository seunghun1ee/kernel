#ifndef __SEM_H
#define __SEM_H



#include <stdbool.h>

typedef struct {
    int value;
    bool init;
} sem_t;

// sem_post and sem_wait assembly code is influence by example code in lecture slide
//http://assets.phoo.org/COMS20001_2019_TB-4/csdsp/os/slide/lec-5-2_s.pdf
//page 7
//Which are originally from
//ARM Synchronization Primitives. Tech. rep. DHT-0008A. ARM Ltd., 2009. url:
//http://infocenter.arm.com/help/topic/com.arm.doc.dht0008a/index.html (see pp. 5, 7).

extern int sem_init(sem_t *sem, int value);
extern int sem_post(sem_t *sem);
extern int sem_wait(sem_t *sem);
extern int sem_destroy(sem_t *sem);

#endif