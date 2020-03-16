/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

/* We assume there will be two user processes, stemming from execution of the
 * two user programs P1 and P2, and can therefore
 *
 * - allocate a fixed-size process table (of PCBs), and then maintain an index
 *   into it to keep track of the currently executing process, and
 * - employ a fixed-case of round-robin scheduling: no more processes can be
 *   created, and neither can be terminated, so assume both are always ready
 *   to execute.
 */

pcb_t procTab[ MAX_PROCS ]; pcb_t* executing = NULL;
proc_stack stack[ MAX_PROCS ];
char* entry_points[ MAX_PROCS ];
uint32_t capn = MAX_PROCS;  //capn = current active process number
uint32_t forkChildPid;

void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {
  char prev_pid = '?', next_pid = '?';

  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) ); // preserve execution context of P_{prev}
    prev_pid = '0' + prev->pid;
  }
  if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) ); // restore  execution context of P_{next}
    next_pid = '0' + next->pid;
  }

    PL011_putc( UART0, '[',      true );
    PL011_putc( UART0, prev_pid, true );
    PL011_putc( UART0, '-',      true );
    PL011_putc( UART0, '>',      true );
    PL011_putc( UART0, next_pid, true );
    PL011_putc( UART0, ']',      true );

    executing = next;                           // update   executing process to P_{next}

  return;
}

int getIndexOfProcTable(pid_t pid) {
  int result;
  for( int i = 0; i < MAX_PROCS; i++) {
    if( pid == procTab[ i ].pid) {
      result = procTab[ i ].pid;
    }
  }
  return result;
}

void schedule( ctx_t* ctx ) {
  int exec;
  int max_priority = 0;
  int next_exec;
  for(int i = 0; i < capn; i++) {
    if(i == getIndexOfProcTable(executing->pid)) {
      exec = i;
      procTab[ i ].priority = procTab[ i ].basePrio;
    }
    else {
      procTab[ i ].priority += 1;
    }

    if(procTab[ i ].priority > max_priority) {
      max_priority = procTab[ i ].priority;
      next_exec = i;
    }
  }
  dispatch( ctx, &procTab[ exec ], &procTab[ next_exec ] );
        procTab[ exec ].status = STATUS_READY;
        procTab[ next_exec ].status = STATUS_EXECUTING;

  
  return;
}



extern void     main_P3();
//extern uint32_t tos_P3;
uint32_t priority_P3 = 3;

extern void     main_P4();
//extern uint32_t tos_P4;
uint32_t priority_P4 = 1;

extern void     main_P1();
//extern uint32_t tos_P1;
uint32_t priority_P1 = 2;

extern void     main_P2();
//extern uint32_t tos_P2;
uint32_t priority_P2 = 0;

extern void     main_P5();
//extern uint32_t tos_P5;
uint32_t priority_P5 = 0;

extern void     main_console();
extern uint32_t tos_console;

extern uint32_t tos_proc;


void initialiseProcTab() {
  for( int i = 0; i < MAX_PROCS; i++ ) {
    procTab[ i ].status = STATUS_INVALID;
    stack[ i ].taken = true;
    stack[ i ].tos = tos_console + ((i+1) * 0x00001000);
  }
  memset( &procTab[ 0 ], 0, sizeof( pcb_t ) ); // initialise 0-th PCB = console
  procTab[ 0 ].pid      = 0;
  procTab[ 0 ].status   = STATUS_READY;
  procTab[ 0 ].tos      = ( uint32_t )( &tos_console  );
  procTab[ 0 ].ctx.cpsr = 0x50;
  procTab[ 0 ].ctx.pc   = ( uint32_t )( &main_console );
  procTab[ 0 ].ctx.sp   = procTab[ 0 ].tos;
  procTab[ 0 ].priority = 4;
  procTab[ 0 ].basePrio = 1;
  /*
  for( int i = 1; i <= MAX_PROCS; i++ ) {
    if(stack[ i ].taken) {
      procTab[ i ].pid     = i;
      procTab[ i ].status  = STATUS_READY;
      procTab[ i ].tos     = ( uint32_t )( stack[ i ].tos );
      procTab[ 0 ].ctx.cpsr = 0x50;
      procTab[ 0 ].ctx.pc   = ( uint32_t )( &main_console );
      procTab[ 0 ].ctx.sp   = procTab[ 0 ].tos;
      procTab[ 0 ].priority = 4;
      procTab[ 0 ].basePrio = 1;
    }
  
  }
  */
}

int findAvailableProcTab() {
  int result;
  for( int i = 0; i < MAX_PROCS; i++) {
    if( procTab[ i ].status == STATUS_INVALID || procTab[ i ].status == STATUS_TERMINATED) {
      result = i;
      break;
    }
  }
  return result;
}


void setProcess(uint32_t pid, const void* entry, int priority) {
  int index = findAvailableProcTab();
  uint32_t forkChildTos = getIndexOfProcTable(forkChildPid);
  procTab[ index ].pid      = pid;
  procTab[ index ].status   = STATUS_READY;
  procTab[ index ].tos      = forkChildTos;
  procTab[ index ].ctx.cpsr = 0x50;
  procTab[ index ].ctx.pc   = ( uint32_t )( &entry );
  procTab[ index ].ctx.sp   = procTab[ index ].tos;
  procTab[ index ].priority = 4;
  procTab[ index ].basePrio = priority;
}



void childProcessInit(uint32_t parentPid, uint32_t childPid) {

  procTab[ parentPid ].ctx.gpr[ 0 ] = childPid;
  memset( &procTab[ childPid ], 0, sizeof( pcb_t ) );
  procTab[ childPid ].pid      = childPid;
  procTab[ childPid ].status   = STATUS_READY;
  procTab[ childPid ].tos      = ( uint32_t )( stack[ childPid ].tos );
  procTab[ childPid ].ctx.cpsr = 0x50;
  procTab[ childPid ].ctx.pc   = procTab[ parentPid ].ctx.pc;
  procTab[ childPid ].ctx.sp   = procTab[ childPid ].tos;
  procTab[ childPid ].priority = procTab[ parentPid ].priority;
  procTab[ childPid ].basePrio = procTab[ parentPid ].basePrio;
  procTab[ childPid ].ctx.gpr[ 0 ] = 0;
}

void doFork() {
  uint32_t parentPid = getIndexOfProcTable(executing->pid);
  uint32_t childPid;
  childPid = findAvailableProcTab();
  childProcessInit(parentPid, childPid);
  forkChildPid = childPid;
}

void hilevel_handler_rst(ctx_t* ctx) {


  /* Invalidate all entries in the process table, so it's clear they are not
 * representing valid (i.e., active) processes.
 */


initialiseProcTab();

/* Automatically execute the user programs P1 and P2 by setting the fields
 * in two associated PCBs.  Note in each case that
 *
 * - the CPSR value of 0x50 means the processor is switched into USR mode,
 *   with IRQ interrupts enabled, and
 * - the PC and SP values match the entry point and top of stack.
 */
/*
memset( &procTab[ 0 ], 0, sizeof( pcb_t ) ); // initialise 0-th PCB = P_3
procTab[ 0 ].pid      = 0;
procTab[ 0 ].status   = STATUS_READY;
procTab[ 0 ].tos      = ( uint32_t )( &tos_P3  );
procTab[ 0 ].ctx.cpsr = 0x50;
procTab[ 0 ].ctx.pc   = ( uint32_t )( &main_P3 );
procTab[ 0 ].ctx.sp   = procTab[ 0 ].tos;
procTab[ 0 ].priority = priority_P3;
procTab[ 0 ].basePrio = priority_P3;

memset( &procTab[ 1 ], 0, sizeof( pcb_t ) ); // initialise 1-st PCB = P_4
procTab[ 1 ].pid      = 1;
procTab[ 1 ].status   = STATUS_READY;
procTab[ 1 ].tos      = ( uint32_t )( &tos_P4  );
procTab[ 1 ].ctx.cpsr = 0x50;
procTab[ 1 ].ctx.pc   = ( uint32_t )( &main_P4 );
procTab[ 1 ].ctx.sp   = procTab[ 1 ].tos;
procTab[ 1 ].priority = priority_P4;
procTab[ 1 ].basePrio = priority_P4;

memset( &procTab[ 2 ], 0, sizeof( pcb_t ) ); // initialise 2-nd PCB = P_1
procTab[ 2 ].pid      = 2;
procTab[ 2 ].status   = STATUS_READY;
procTab[ 2 ].tos      = ( uint32_t )( &tos_P1  );
procTab[ 2 ].ctx.cpsr = 0x50;
procTab[ 2 ].ctx.pc   = ( uint32_t )( &main_P1 );
procTab[ 2 ].ctx.sp   = procTab[ 2 ].tos;
procTab[ 2 ].priority = priority_P1;
procTab[ 2 ].basePrio = priority_P1;

memset( &procTab[ 3 ], 0, sizeof( pcb_t ) ); // initialise 3-rd PCB = P_2
procTab[ 3 ].pid      = 3;
procTab[ 3 ].status   = STATUS_READY;
procTab[ 3 ].tos      = ( uint32_t )( &tos_P2  );
procTab[ 3 ].ctx.cpsr = 0x50;
procTab[ 3 ].ctx.pc   = ( uint32_t )( &main_P2 );
procTab[ 3 ].ctx.sp   = procTab[ 3 ].tos;
procTab[ 3 ].priority = priority_P2;
procTab[ 3 ].basePrio = priority_P2;

memset( &procTab[ 4 ], 0, sizeof( pcb_t ) ); // initialise 4-th PCB = P_5
procTab[ 4 ].pid      = 4;
procTab[ 4 ].status   = STATUS_READY;
procTab[ 4 ].tos      = ( uint32_t )( &tos_P5  );
procTab[ 4 ].ctx.cpsr = 0x50;
procTab[ 4 ].ctx.pc   = ( uint32_t )( &main_P5 );
procTab[ 4 ].ctx.sp   = procTab[ 4 ].tos;
procTab[ 4 ].priority = priority_P5;
procTab[ 4 ].basePrio = priority_P5;

memset( &procTab[ 5 ], 0, sizeof( pcb_t ) ); // initialise 5-th PCB = console
procTab[ 5 ].pid      = 5;
procTab[ 5 ].status   = STATUS_READY;
procTab[ 5 ].tos      = ( uint32_t )( &tos_console  );
procTab[ 5 ].ctx.cpsr = 0x50;
procTab[ 5 ].ctx.pc   = ( uint32_t )( &main_console );
procTab[ 5 ].ctx.sp   = procTab[ 5 ].tos;
procTab[ 5 ].priority = 4;
procTab[ 5 ].basePrio = 1;
*/

int loadedP = 0;
for(int i = 0; i < MAX_PROCS; i++ ) {
  if(procTab[ i ].status == STATUS_READY) {
    loadedP += 1;
  }
}
capn = loadedP;


/* Once the PCBs are initialised, we arbitrarily select the 0-th PCB to be
 * executed: there is no need to preserve the execution context, since it
 * is invalid on reset (i.e., no process was previously executing).
 */

dispatch( ctx, NULL, &procTab[ 0 ] );

/* Configure the mechanism for interrupt handling by
 *
 * - configuring timer st. it raises a (periodic) interrupt for each
 *   timer tick,
 * - configuring GIC st. the selected interrupts are forwarded to the
 *   processor via the IRQ interrupt signal, then
 * - enabling IRQ interrupts.
 */

TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

GICC0->PMR          = 0x000000F0; // unmask all            interrupts
GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
GICC0->CTLR         = 0x00000001; // enable GIC interface
GICD0->CTLR         = 0x00000001; // enable GIC distributor

int_enable_irq();

  return;
}

void hilevel_handler_irq(ctx_t* ctx) {
  // Step 2: read  the interrupt identifier so we know the source.

  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.

  if( id == GIC_SOURCE_TIMER0 ) {
    PL011_putc( UART0, 'T', true ); TIMER0->Timer1IntClr = 0x01;
    schedule( ctx );
  }

  // Step 5: write the interrupt identifier to signal we're done.

  GICC0->EOIR = id;

  return;
}

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {
  /* Based on the identifier (i.e., the immediate operand) extracted from the
   * svc instruction,
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call, then
   * - write any return value back to preserved usr mode registers.
   */

  switch( id ) {
    case 0x00 : { // 0x00 => yield()
      //schedule( ctx );

      break;
    }

    case 0x01 : { // 0x01 => write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++, true );
      }

      ctx->gpr[ 0 ] = n;

      break;
    }

    case 0x03 : { //0x03 => fork
      doFork();
      PL011_putc( UART0, 'F', true );
    }

    case 0x04 : { //0x04 => exit
      
      PL011_putc( UART0, 'E', true );
    }

    case 0x05 : { //0x05 => exec( x )
      const void*  x = ( const void* )( ctx->gpr[ 0 ] );
      setProcess(forkChildPid, x, 0);
      schedule( ctx );
      PL011_putc( UART0, 'X', true );
    }

    case 0x06 : { //0x06 => kill
      PL011_putc( UART0, 'K', true );
    }

    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}
