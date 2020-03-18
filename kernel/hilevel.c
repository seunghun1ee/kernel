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
int readyPcbIndex[ MAX_PROCS ];
uint32_t capn = MAX_PROCS;  //capn = current available process number
uint32_t forkChildPid;

void updateCapnAndReadyIndex() {
  int loadedP = 0;
  int j = 0;
  for(int i = 0; i < MAX_PROCS; i++ ) {
    if(procTab[ i ].status == STATUS_READY || procTab[ i ].status == STATUS_EXECUTING ) {
      loadedP += 1;
      readyPcbIndex[j] = i;
      j++;
    }
  }
  capn = loadedP;
}

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
      break;
    }
  }
  return result;
}

void schedule( ctx_t* ctx ) {
  int exec;
  int max_priority = 0;
  int next_exec;
  for(int i = 0; i < capn; i++) {
    if(readyPcbIndex[i] == getIndexOfProcTable(executing->pid)) {
      exec = readyPcbIndex[i];
      procTab[ readyPcbIndex[i] ].priority = procTab[ readyPcbIndex[i] ].basePrio;
    }
    else {
      procTab[ readyPcbIndex[i] ].priority += 1;
    }

    if(procTab[ readyPcbIndex[i] ].priority > max_priority) {
      max_priority = procTab[ readyPcbIndex[i] ].priority;
      next_exec = readyPcbIndex[i];
    }
  }
  procTab[ exec ].status = STATUS_READY;
  procTab[ next_exec ].status = STATUS_EXECUTING;
  dispatch( ctx, &procTab[ exec ], &procTab[ next_exec ] );
  

  
  return;
}



extern void     main_P3();
extern void     main_P4();
extern void     main_P5();

extern void     main_console();

extern uint32_t tos_svc;
extern uint32_t tos_proc;


void initialiseProcTab() {
  for( int i = 0; i < MAX_PROCS; i++ ) {
    procTab[ i ].status = STATUS_INVALID;
    stack[ i ].taken = false;
    stack[ i ].tos = tos_svc + ((i+1) * 0x00001000);
  }
  //initialise console
  stack[ 0 ].taken = true;
  stack[ 0 ].pid = 0;
  memset( &procTab[ 0 ], 0, sizeof( pcb_t ) ); // initialise 0-th PCB = console
  procTab[ 0 ].pid      = 0;
  procTab[ 0 ].status   = STATUS_READY;
  procTab[ 0 ].tos      = stack[ 0 ].tos;
  procTab[ 0 ].ctx.cpsr = 0x50;
  procTab[ 0 ].ctx.pc   = ( uint32_t )( &main_console );
  procTab[ 0 ].ctx.sp   = procTab[ 0 ].tos;
  procTab[ 0 ].priority = 4;
  procTab[ 0 ].basePrio = 0;
}

int findAvaialbeTos() {
  int result;
  for (int i = 0; i < MAX_PROCS; i++) {
    if( stack[ i ].taken == false) {
      result = i;
      break;
    }
  }
  return result;
}

void setStack(int i, pid_t pid) {
  stack[ i ].taken = true;
  stack[ i ].pid = pid;
}

int getIndexOfStackByTos(uint32_t tosAddress) {
  int result;
  for ( int i = 0; i < MAX_PROCS; i++) {
    if( stack[ i ].tos == tosAddress && stack[i].taken) {
      result = i;
      break;
    }
  }
  return result;
}

int getIndexOfStackByPid(pid_t pid) {
  int result;
  for ( int i = 0; i < MAX_PROCS; i++) {
    if( stack[ i ].pid == pid && stack[i].taken) {
      result = i;
      break;
    }
  }
  return result;
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

void setProcess(pcb_t pcb, uint32_t pid, status_t status, uint32_t tos, ctx_t context, int priority, int basePriority) {
  memset( &pcb, 0, sizeof(pcb_t));
  pcb.pid      = pid;
  pcb.status   = status;
  pcb.tos      = tos;
  pcb.priority = priority;
  pcb.basePrio = basePriority;
}

void initEmptyPcb(pcb_t pcb, uint32_t pid, status_t status) {
  pcb.pid = pid;
  pcb.status = status;
}


void hilevel_fork(ctx_t *ctx) {
  pid_t parentPid = getIndexOfProcTable(executing->pid);
  pid_t childPid = findAvailableProcTab();
  procTab[childPid].pid = childPid;
  
  int parentProcTabIndex = getIndexOfProcTable(parentPid);
  int childProcTabIndex = childPid;

  int parentStackIndex = getIndexOfStackByPid(parentPid);
  int childStackIndex = findAvaialbeTos();


  setStack(childStackIndex, childPid);
  memset(&procTab[childProcTabIndex], 0, sizeof(pcb_t));
  procTab[childPid].pid = childPid;
  procTab[childPid].status = STATUS_READY;
  procTab[childPid].tos = (uint32_t) stack[childStackIndex].tos;
  procTab[childPid].priority = procTab[parentProcTabIndex].priority;
  procTab[childPid].basePrio = procTab[parentProcTabIndex].basePrio;
  procTab[childPid].parent = parentPid;

  memcpy((uint32_t) stack[ childStackIndex ].tos - 0x00001000, (uint32_t) stack[ parentStackIndex ].tos - 0x00001000, sizeof( 0x00001000 ));
  memcpy((uint32_t) &procTab[ childProcTabIndex ].ctx, ctx, sizeof(ctx_t));

  
  uint32_t sp_offset = (uint32_t) procTab[ parentProcTabIndex ].tos - ctx->sp;
  procTab[ childProcTabIndex ].ctx.sp = (uint32_t) procTab[childProcTabIndex].tos - sp_offset;
  ctx->gpr[ 0 ] = childPid;
  procTab[ childProcTabIndex ].ctx.gpr[ 0 ] = 0;
  

}

void hilevel_exit(ctx_t *ctx, int exit_status) {
  pid_t currentPid = executing->pid;
  int currentProcTabIndex = getIndexOfProcTable(currentPid);
  int currentStackIndex = getIndexOfStackByPid(currentPid);
  int parentProcTabIndex = getIndexOfProcTable(procTab[currentProcTabIndex].parent);
  if(exit_status == EXIT_SUCCESS) {
    procTab[ currentProcTabIndex ].status = STATUS_TERMINATED;
    stack[ currentStackIndex ].taken = false;
    updateCapnAndReadyIndex();
  }
}

void hilevel_exec(ctx_t *ctx, void* program) {
  int programStackIndex = findAvaialbeTos();
  int currentPidProcTabIndex = getIndexOfProcTable(executing->pid);
  setStack(programStackIndex, executing->pid);

  ctx->pc = (uint32_t) program;
  ctx->sp = (uint32_t) procTab[ currentPidProcTabIndex ].tos;
}

void hilevel_kill(ctx_t *ctx, int pid, int signal) {
  uint32_t sys_signal = signal & 0xFF;
  int procTabIndex = getIndexOfProcTable((pid_t) pid);
  int stackIndex = getIndexOfStackByPid((pid_t) pid);
  int parentProcTabIndex = getIndexOfProcTable(procTab[procTabIndex].parent);
  procTab[ procTabIndex ].status = STATUS_TERMINATED;
  stack[ stackIndex ].taken = false;
  updateCapnAndReadyIndex();
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
*/
updateCapnAndReadyIndex();


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
    updateCapnAndReadyIndex();
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
    case SYS_YIELD : { // 0x00 => yield()
      schedule( ctx );
      break;
    }

    case SYS_WRITE : { // 0x01 => write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++, true );
      }

      ctx->gpr[ 0 ] = n;

      break;
    }

    case SYS_READ : { // 0x02 => read( fd, x, n )
      break;
    }

    case SYS_FORK : { //0x03 => fork
      hilevel_fork(ctx);
      PL011_putc( UART0, 'F', true );
      break;
    }

    case SYS_EXIT : { //0x04 => exit( x )
      int x = (uint32_t) ctx->gpr[ 0 ];
      hilevel_exit(ctx, x);
      PL011_putc( UART0, 'E', true );
      break;
    }

    case SYS_EXEC : { //0x05 => exec( x )
      void *x = (uint32_t) ctx->gpr[ 0 ];
      hilevel_exec(ctx, x);
      PL011_putc( UART0, 'X', true );
      break;
    }

    case SYS_KILL : { //0x06 => kill
      int pid = (uint32_t) ctx->gpr[ 0 ];
      int x   = (uint32_t) ctx->gpr[ 1 ];
      hilevel_kill(ctx, pid, x);
      PL011_putc( UART0, 'K', true );
      break;
    }

    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}
