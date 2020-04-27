#include "IPC.h"

void sem_init(sem_t *sem, int value) {
  asm volatile( "mov r0, %0 \n"
                "mov r1, %1 \n"
                "str r1, [ r0 ]\n"
                :
                : "r" (sem), "r" (value)
                : "r0", "r1" );
  return;
}

int sem_post(sem_t *sem) {
  int r;

  asm volatile( "mov   r0, %1         \n"  // assign r0 = sem
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
                : );
  return r;
}

int sem_wait(sem_t *sem) {
  int r;

  asm volatile( "mov   r0, %1         \n"  // assign r0 = sem
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
                : );
  return r;
}

void sem_destroy(sem_t *sem) {
  asm volatile( "mov r0, %0 \n"
                "mvn r1, #0 \n"
                "str r1, [ r0 ]\n"
                :
                : "r" (sem)
                : "r0" );
  return;
}