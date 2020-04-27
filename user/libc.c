/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "libc.h"

int  atoi( char* x        ) {
  char* p = x; bool s = false; int r = 0;

  if     ( *p == '-' ) {
    s =  true; p++;
  }
  else if( *p == '+' ) {
    s = false; p++;
  }

  for( int i = 0; *p != '\x00'; i++, p++ ) {
    r = s ? ( r * 10 ) - ( *p - '0' ) :
            ( r * 10 ) + ( *p - '0' ) ;
  }

  return r;
}

void itoa( char* r, int x ) {
  char* p = r; int t, n;

  if( x < 0 ) {
     p++; t = -x; n = t;
  }
  else {
          t = +x; n = t;
  }

  do {
     p++;                    n /= 10;
  } while( n );

    *p-- = '\x00';

  do {
    *p-- = '0' + ( t % 10 ); t /= 10;
  } while( t );

  if( x < 0 ) {
    *p-- = '-';
  }

  return;
}

void yield() {
  asm volatile( "svc %0     \n" // make system call SYS_YIELD
              :
              : "I" (SYS_YIELD)
              : );

  return;
}

int write( int fd, const void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_WRITE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r) 
              : "I" (SYS_WRITE), "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int  read( int fd,       void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_READ
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r) 
              : "I" (SYS_READ),  "r" (fd), "r" (x), "r" (n) 
              : "r0", "r1", "r2" );

  return r;
}

int  fork() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (SYS_FORK)
              : "r0" );

  return r;
}

void exit( int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %0     \n" // make system call SYS_EXIT
              :
              : "I" (SYS_EXIT), "r" (x)
              : "r0" );

  return;
}

void exec( const void* x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 = x
                "svc %0     \n" // make system call SYS_EXEC
              :
              : "I" (SYS_EXEC), "r" (x)
              : "r0" );

  return;
}

int  kill( int pid, int x ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  pid
                "mov r1, %3 \n" // assign r1 =    x
                "svc %1     \n" // make system call SYS_KILL
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r) 
              : "I" (SYS_KILL), "r" (pid), "r" (x)
              : "r0", "r1" );

  return r;
}

void nice( int pid, int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  pid
                "mov r1, %2 \n" // assign r1 =    x
                "svc %0     \n" // make system call SYS_NICE
              : 
              : "I" (SYS_NICE), "r" (pid), "r" (x)
              : "r0", "r1" );

  return;
}

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



