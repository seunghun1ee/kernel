/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __HILEVEL_H
#define __HILEVEL_H

// Include functionality relating to newlib (the standard C library).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>

// Include functionality relating to the platform.

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"
#include "PL050.h"
#include "PL111.h"
#include   "SYS.h"
#include "console.h"


// Include functionality relating to the   kernel.

#include "lolevel.h"
#include     "int.h"


/* The kernel source code is made simpler and more consistent by using
 * some human-readable type definitions:
 *
 * - a type that captures a Process IDentifier (PID), which is really
 *   just an integer,
 * - an enumerated type that captures the status of a process, e.g.,
 *   whether it is currently executing,
 * - a type that captures each component of an execution context (i.e.,
 *   processor state) in a compatible order wrt. the low-level handler
 *   preservation and restoration prologue and epilogue, and
 * - a type that captures a process PCB.
 */


#define MAX_PROCS 20
#define STACK_SIZE 0x00001000
#define MAX_PIPES 20
#define QUEUE_LEN 1000

typedef int pid_t;

typedef enum {
  STATUS_INVALID,

  STATUS_CREATED,
  STATUS_TERMINATED,

  STATUS_READY,
  STATUS_EXECUTING,
  STATUS_WAITING
} status_t;

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef struct {
     pid_t    pid; // Process IDentifier (PID)
  status_t status; // current status
  uint32_t    tos; // address of Top of Stack (ToS)
     ctx_t    ctx; // execution context
  int32_t priority; // Priority of process
  int32_t basePrio; // Base Priority of process
    pid_t  parent;  //pid of parent process
} pcb_t;

typedef struct {
  uint32_t tos; // Top of the stack address
  bool     taken; //Boolean for checking if the stack slot is taken
  pid_t pid;   //pid of the process uses the stack address
} proc_stack;

typedef struct {
    int read_end;
    int write_end;
    char queue[ QUEUE_LEN ];
    int front;
    int back;
    int length;
    int itemCount;
    bool taken;
} pipe_t;

typedef struct {
    int pipeIndex;
    bool taken;
} file_descriptor_t;

typedef enum {
  OP_ADD,
  OP_SUB
} update_display_op;


#endif
