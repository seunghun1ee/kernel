#include "sem.h"

// sem_post and sem_wait assembly code is influence by example code in lecture slide
//http://assets.phoo.org/COMS20001_2019_TB-4/csdsp/os/slide/lec-5-2_s.pdf
//page 7
//Which are originally from
//ARM Synchronization Primitives. Tech. rep. DHT-0008A. ARM Ltd., 2009. url:
//http://infocenter.arm.com/help/topic/com.arm.doc.dht0008a/index.html (see pp. 5, 7).

int sem_init(sem_t *sem, int value) {
  int r;

  asm volatile( "mov r0, %1           \n"  //assign r0 = sem
                "mov r1, %2           \n"  //assign r1 = value
                "ldr r2, [ r0, #4 ]   \n"  //load sem.init to r2
                "cmp r2, #0           \n"  //check if the semaphore is not initialised before
                "streq r1, [ r0 ]     \n"  //store value to the semaphore
                "addeq r2, r2, #1     \n"  //new sem.init value true
                "streq r2, [ r0, #4 ] \n"  //update sem.init
                "moveq %0, #0         \n"  //assign r = 0
                "movne %0, #1         \n"  //assign r = 1
                : "=r" (r)
                : "r" (sem), "r" (value)
                : "r0", "r1" );
  return r;
}

int sem_post(sem_t *sem) {
  int r;

  asm volatile( "mov   r0, %1         \n"  //assign r0 = sem
                "ldrex r1, [ r0 ]     \n"  //load from semaphores's address exclusively
                "mov   r2, #1         \n"  //move error code 1 to r2
                "cmp   r1, #0         \n"  //check the semaphore value
                "blt   post_error     \n"  //if the value is negative, branch to error
                "add   r1, r1, #1     \n"  //add 1 to semaphore value -> 0 + 1 = 1
                "strex r2, r1, [ r0 ] \n"  //store new semaphore value exclusively at the address
                "cmp   r2, #0         \n"  //check if the strex succeded
                "bne   sem_post       \n"  //restart if strex was not successful
    "post_error: mov   %0, r2         \n"  //assigin r = r2
                "dmb                  \n"  //data block
                "bx    lr             \n"  //go back to process
                : "=r" (r)
                : "r" (sem)
                : "r0" );
  return r;
}

int sem_wait(sem_t *sem) {
  int r;

  asm volatile( "mov   r0, %1         \n"  //assign r0 = sem
                "ldrex r1, [ r0 ]     \n"  //load from semaphore's address exclusively
                "mov   r2, #1         \n"  //move error code 1 to r2
                "cmp   r1, #0         \n"  //check the semaphore value
                "blt   wait_error     \n"  //if the value is negative branch to error
                "beq   sem_wait       \n"  //if the value is 0, restart
                "sub   r1, r1, #1     \n"  //subtract 1 from semaphore value -> 1 - 1 = 0
                "strex r2, r1, [ r0 ] \n"  //store new semaphore value exclusively at the address
                "cmp   r2, #0         \n"  //check if the strex succeded
                "bne   sem_wait       \n"  //restart if strex was not successful
    "wait_error: mov   %0, r2         \n"  //assign r = r2
                "dmb                  \n"  //data block
                "bx    lr             \n"  //go back to process
                : "=r" (r)
                : "r" (sem)
                : "r0" );
  return r;
}

int sem_destroy(sem_t *sem) {
  int r;

  asm volatile( "mov r0, %1           \n"  //asign r0 = sem
                "ldr r2, [ r0, #4 ]   \n"  //load sem.init to r2
                "cmp r2, #1           \n"  //check if the semaphore is initialised before
                "mvneq r1, #0         \n"  //move -1 to r1
                "streq r1, [ r0 ]     \n"  //store new semaphore value -1
                "subeq r2, r2, #1     \n"  //new sem.init value false
                "streq r2, [ r0, #4 ] \n"  //update sem.init
                "moveq %0, #0         \n"  //assign r = 0
                "movne %0, #1         \n"  //assign r = 1
                : "=r" (r)
                : "r" (sem)
                : "r0" );
  return r;
}