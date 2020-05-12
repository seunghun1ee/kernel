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
pipe_t pipes[MAX_PIPES];
file_descriptor_t fd[50];

extern void     main_console();
extern uint32_t tos_svc;
extern uint32_t tos_proc;

uint16_t fb[ 600 ][ 800 ];
uint16_t currentX = 0;
uint16_t currentY = 0;
char **char_set;
uint8_t prev_ps20_id = 0x0;
bool shift_key = false;

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

/* Invalidate all entries in the process table, so it's clear they are not
 * representing valid (i.e., active) processes.
 */
void initialiseProcTab() {
  for( int i = 0; i < MAX_PROCS; i++ ) {
    procTab[ i ].status = STATUS_INVALID;
    stack[ i ].taken = false;
    stack[ i ].tos = (uint32_t) &tos_proc - (i * STACK_SIZE);
  }
  //initialise console
/* 
 * - the CPSR value of 0x50 means the processor is switched into USR mode,
 *   with IRQ interrupts enabled, and
 * - the PC and SP values match the entry point and top of stack.
 */
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

void initFd() {
  for(int i = 0; i < 3; i++) {
    fd[i].pipeIndex = -1;
    fd[i].taken = true;
  }
  for(int i = 3; i < 50; i++) {
    fd[i].pipeIndex = -1;
    fd[i].taken = false;
  }
}

void initPipes() {
  for(int i = 0; i < MAX_PIPES; i++) {
    pipes[i].read_end = -1;
    pipes[i].write_end = -1;
    pipes[i].taken = false;
    for(int j = 0; j < QUEUE_LEN; j++) {
      pipes[i].queue[j] = '\0';
    }
    pipes[i].front = -1;
    pipes[i].back = -1;
    pipes[i].itemCount = 0;
    pipes[i].length = QUEUE_LEN;
  }
}

void initDisplay() {
  for( int i = 0; i < 600; i++ ) {
    for( int j = 0; j < 800; j++ ) {
      fb[ i ][ j ] = 0x0;
    }
  }
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

int findAvailableFd() {
  int result = -1;
  for(int i = 3; i < 50; i++) {
    if(fd[i].taken == false) {
      result = i;
      break;
    }
  }
  return result;
}

int findAvailablePipe() {
  int result = -1;
  for(int i = 0; i < MAX_PIPES; i++) {
    if(!pipes[i].taken) {
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

//push and pop functions are influenced by the tutorial from
//https://www.codesdope.com/blog/article/making-a-queue-using-an-array-in-c/

int push(int index, char item) {
  if(item == '\0') {
    return 0;  //null char ignored
  }
  if(pipes[index].itemCount < pipes[index].length) {
    if(pipes[index].itemCount == 0) {
      pipes[index].queue[0] = item;
      
      pipes[index].front = 0;
      pipes[index].back = 0;
    }
    else if(pipes[index].back == pipes[index].length -1 ) {
      pipes[index].queue[0] = item;
      pipes[index].back = 0;
    }
    else {
      pipes[index].back++;
      pipes[index].queue[pipes[index].back] = item;
    }
    pipes[index].itemCount++;
    return 1;  //push success
  }
  
  //queue is full
  return -1;
  
  
}

char pop(int index) {
  if(pipes[index].itemCount > 0) {
    char item = pipes[index].queue[pipes[index].front];
    if(item == '\0') {
      return 0; //ignore null char
    }

    pipes[index].itemCount--;
    pipes[index].front++;
    return item;  //pop success
  }
  
  //queue is empty
  return 0;
}

void lineFeed() {
  currentY += 10;
  currentX = 0;
}


void backspace() {
  if(currentX == 0 && currentY == 0) {
    return;
  }
  
  if(currentX == 0) {
    if(currentY != 0) {
      currentY -= 10;
      currentX = 792;
    }
  }
  else {
    currentX -= 8;
  }

  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      fb[ currentY + i ][ currentX + j ] = 0x0; 
    }
  }
}

void updateXY(update_display_op op) {
  uint8_t tempX = currentX;
  uint8_t tempY = currentY;
  switch (op)
  {
  case OP_ADD:
    currentX += 8;
    if(currentX >= 800) {
      lineFeed();
    }
    break;
  
  case OP_SUB:
    backspace();
    break;
  default:
    break;
  }
  
}

void putChar(uint16_t x, uint16_t y, char character, uint16_t colour) {
  char *bitmap = char_set[character];
  switch (character) {
    case '\n':
      lineFeed();
      break;
  
    default:
      for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
          switch(bitmap[(j*8)+i]) {
            case '1': {
              fb[y+j][x+i] = colour;
              break;
            }
            default: {
              break;
            }
          }
        }
      }
      updateXY(OP_ADD);
      break;
  }
}

void hilevel_yield(ctx_t *ctx) {
  updateCapnAndReadyIndex();
  schedule( ctx );
}

void hilevel_write(ctx_t *ctx, int fdIndex, char *x, int n) {
  int success = 0;
  switch (fdIndex) {
    case STDIN_FILENO ... STDOUT_FILENO:
      for(int i = 0; i < n; i++ ) {
        char a = *x;
        if(a >= 'a' && a <= 'z') {
          a -= 32;
        }
        putChar(currentX, currentY, a, 0x7FFF);
        PL011_putc( UART1, *x++, true );
        success++;
      }
      break;

    case STDERR_FILENO:
      PL011_putc( UART1, '[', true );
      PL011_putc( UART1, 'E', true );
      PL011_putc( UART1, 'R', true );
      PL011_putc( UART1, 'R', true );
      PL011_putc( UART1, 'O', true );
      PL011_putc( UART1, 'R', true );
      PL011_putc( UART1, ']', true );
      for(int i = 0; i < n; i++ ) {
        PL011_putc( UART1, *x++, true );
        success++;
      }
      break;

    default:
      if(fdIndex == pipes[fd[fdIndex].pipeIndex].write_end) {
        for(int i = 0; i < n; i++ ) {
          int push_err = push(fd[fdIndex].pipeIndex, x[i]);
          if(push_err > 0) {  //on success
            success++;
          }
        }
      }
    
      break;
  }

  ctx->gpr[ 0 ] = success;
}

void hilevel_read(ctx_t *ctx, int fdIndex, char *x, int n) {
  int success = 0;
  switch (fdIndex) {
    case STDIN_FILENO ... STDOUT_FILENO:
      for( int i = 0; i < n; i++ ) {
        *x++ = PL011_getc(UART1, true);
        success++;
      }
      break;

    case STDERR_FILENO:
      PL011_putc( UART1, '[', true );
      PL011_putc( UART1, 'E', true );
      PL011_putc( UART1, 'R', true );
      PL011_putc( UART1, 'R', true );
      PL011_putc( UART1, 'O', true );
      PL011_putc( UART1, 'R', true );
      PL011_putc( UART1, ']', true );
      for(int i = 0; i < n; i++ ) {
        PL011_putc( UART1, *x++, true );
        success++;
      }
      break;

    default:
      if(fdIndex == pipes[fd[fdIndex].pipeIndex].read_end) {
        for( int i = 0; i < n; i++ ) {
          char pop_err = x[i] = pop(fd[fdIndex].pipeIndex);
          if(pop_err > 0) {  //on success
            success++;
          }
        }
      }
    
      break;
  }
  

  ctx->gpr[ 0 ] = success;
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
  procTab[childProcTabIndex].pid = childPid;
  procTab[childProcTabIndex].status = STATUS_READY;
  procTab[childProcTabIndex].priority = procTab[parentProcTabIndex].priority;
  procTab[childProcTabIndex].basePrio = procTab[parentProcTabIndex].basePrio;
  procTab[childProcTabIndex].parent = parentPid;
  procTab[childProcTabIndex].tos = (uint32_t) stack[childStackIndex].tos;

  memcpy((void *) stack[ childStackIndex ].tos - STACK_SIZE, (const void *) stack[ parentStackIndex ].tos - STACK_SIZE, STACK_SIZE);
  memcpy((void *) &procTab[ childProcTabIndex ].ctx, (const void *) ctx, sizeof(ctx_t));

  
  uint32_t sp_offset = (uint32_t) procTab[ parentProcTabIndex ].tos - ctx->sp;
  procTab[ childProcTabIndex ].ctx.sp = (uint32_t) procTab[childProcTabIndex].tos - sp_offset;
  ctx->gpr[ 0 ] = childPid;
  procTab[ childProcTabIndex ].ctx.gpr[ 0 ] = 0;
  
  //
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
    schedule(ctx);
  }
}

void hilevel_exec(ctx_t *ctx, void* program) {
  ctx->pc = (uint32_t) program;
  ctx->sp = executing->tos;
}

void hilevel_kill(ctx_t *ctx, int pid, int signal) {
  uint32_t sys_signal = signal & 0xFF;
  int procTabIndex = getIndexOfProcTable((pid_t) pid);
  int stackIndex = getIndexOfStackByPid((pid_t) pid);
  int parentProcTabIndex = getIndexOfProcTable(procTab[procTabIndex].parent);
  procTab[ procTabIndex ].status = STATUS_TERMINATED;
  stack[ stackIndex ].taken = false;
  updateCapnAndReadyIndex();
  schedule(ctx);
}

void hilevel_nice(int pid, int32_t inc) {
  int pidProcTabIndex = getIndexOfProcTable(pid);
  procTab[pidProcTabIndex].priority += inc;
}

void hilevel_pipe(ctx_t *ctx) {
  int readIndex = findAvailableFd();
  if(readIndex < 0) {
    ctx->gpr[ 3 ] = -1;  //no avilable fd
    return;
  }
  fd[readIndex].taken = true;
  
  int writeIndex = findAvailableFd();
  if(writeIndex < 0) {
    ctx->gpr[ 3 ] = -1;  //no available fd
    return;
  }
  fd[writeIndex].taken = true;
  
  int pipeIndex = findAvailablePipe();
  if(pipeIndex < 0) {
    ctx->gpr[ 3 ] = -1;  //no available pipe
    return;
  }

  pipes[pipeIndex].read_end = readIndex;
  pipes[pipeIndex].write_end = writeIndex;
  pipes[pipeIndex].taken = true;

  fd[readIndex].pipeIndex = pipeIndex;
  fd[writeIndex].pipeIndex = pipeIndex;
  
  ctx->gpr[ 3 ] = 0;
  ctx->gpr[ 1 ] = readIndex;
  ctx->gpr[ 2 ] = writeIndex;
}

void hilevel_close(ctx_t *ctx, int fdIndex) {
  if(!fd[fdIndex].taken) {
    //error it's not opened before
    ctx->gpr[ 3 ] = -1;
    return;
  }
  if(fdIndex == pipes[fd[fdIndex].pipeIndex].read_end) {
    pipes[fd[fdIndex].pipeIndex].read_end = -1;
  }
  else if(fdIndex == pipes[fd[fdIndex].pipeIndex].write_end) {
    pipes[fd[fdIndex].pipeIndex].write_end = -1;
  }
  else {
    //error wrong file descriptor
    ctx->gpr[ 3 ] = -1;
    return;
  }

  if(pipes[fd[fdIndex].pipeIndex].read_end == -1 && pipes[fd[fdIndex].pipeIndex].write_end == -1) {
    pipes[fd[fdIndex].pipeIndex].taken = false;  //when both ends are closed, the pipe is freed
  }

  fd[fdIndex].pipeIndex = -1;
  fd[fdIndex].taken = false;

  ctx->gpr[ 3 ] = 0;
}

void keyboard_behaviour_0(uint8_t id) {
  switch (id) {
    case 0x29:
      //space bar
      updateXY(OP_ADD);
      break;
    case 0x66:
      //backspace
      updateXY(OP_SUB);
      break;
    case 0x5A:
      //enter key
      PL050_putc(PS20,'\x0A');
      //UART1->FR = 0x90;
      lineFeed();
      break;
    case 0x12:
    case 0x59:
      if(shift_key) {
        shift_key = false;
      }
      else
      {
        shift_key = true;
      }
      break;
    case 0x5D:
      if(shift_key) {
        putChar(currentX,currentY,'~',0x7FFF);  
      }
      else {
        putChar(currentX,currentY,'#',0x7FFF);
      }
      break;
    case 0x52:
      if(shift_key) {
        putChar(currentX,currentY,'@',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'\'',0x7FFF);
      }
      break;
    case 0x41:
      if(shift_key) {
        putChar(currentX,currentY,'<',0x7FFF);
      }
      else {
        putChar(currentX,currentY,',',0x7FFF);
      }
      break;
    case 0x4E:
      if(shift_key) {
        putChar(currentX,currentY,'_',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'-',0x7FFF);
      }
      break;
    case 0x49:
      if(shift_key) {
        putChar(currentX,currentY,'>',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'.',0x7FFF);
      }
      break;
    case 0x4A:
      if(shift_key) {
        putChar(currentX,currentY,'?',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'/',0x7FFF);
      }
      break;

    case 0x45:
      if(shift_key) {
        putChar(currentX,currentY,')',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'0',0x7FFF);
      }
      break;
    case 0x16:
      if(shift_key) {
        putChar(currentX,currentY,'!',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'1',0x7FFF);
      }
      break;
    case 0x1E:
      if(shift_key) {
        putChar(currentX,currentY,'\"',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'2',0x7FFF);
      }
      break;
    case 0x26:
      if(shift_key) {
        putChar(currentX,currentY,(unsigned char) 156,0x7FFF);
      }
      else {
        putChar(currentX,currentY,'3',0x7FFF);
      }
      break;
    case 0x25:
      if(shift_key) {
        putChar(currentX,currentY,'$',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'4',0x7FFF);
      }
      break;
    case 0x2E:
      if(shift_key) {
        putChar(currentX,currentY,'%',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'5',0x7FFF);
      }
      break;
    case 0x36:
      if(shift_key) {
        putChar(currentX,currentY,'^',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'6',0x7FFF);
      }
      break;
    case 0x3D:
      if(shift_key) {
        putChar(currentX,currentY,'&',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'7',0x7FFF);
      }
      break;
    case 0x3E:
      if(shift_key) {
        putChar(currentX,currentY,'*',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'8',0x7FFF);
      }
      break;
    case 0x46:
      if(shift_key) {
        putChar(currentX,currentY,'(',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'9',0x7FFF);
      }
      break;
    case 0x4C:
      if(shift_key) {
        putChar(currentX,currentY,':',0x7FFF);
      }
      else {
        putChar(currentX,currentY,';',0x7FFF);
      }
      break;                    
    case 0x1C:
      putChar(currentX,currentY,'A',0x7FFF);
      //PL011_putc(UART1, 'A', true);
      PL050_putc(PS20, (uint8_t) 'A');
      PL011_putc(UART1, PL050_getc(PS20) & 0xFF, true);
      break;
    case 0x32:
      putChar(currentX,currentY,'B',0x7FFF);
      break;
    case 0x21:
      putChar(currentX,currentY,'C',0x7FFF);  
      break;
    case 0x23:
      putChar(currentX,currentY,'D',0x7FFF);
      break;
    case 0x24:
      putChar(currentX,currentY,'E',0x7FFF);
      break;
    case 0x2B:
      putChar(currentX,currentY,'F',0x7FFF);
      break;
    case 0x34:
      putChar(currentX,currentY,'G',0x7FFF);
      break;
    case 0x33:
      putChar(currentX,currentY,'H',0x7FFF);
      break;        
    case 0x43:
      putChar(currentX,currentY,'I',0x7FFF);
      break;
    case 0x3B:
      putChar(currentX,currentY,'J',0x7FFF);
      break;
    case 0x42:
      putChar(currentX,currentY,'K',0x7FFF);
      break;
    case 0x4B:
      putChar(currentX,currentY,'L',0x7FFF);
      break;
    case 0x3A:
      putChar(currentX,currentY,'M',0x7FFF);
      break;
    case 0x31:
      putChar(currentX,currentY,'N',0x7FFF);
      break;
    case 0x44:
      putChar(currentX,currentY,'O',0x7FFF);
      break;
    case 0x4D:
      putChar(currentX,currentY,'P',0x7FFF);
      break;
    case 0x15:
      putChar(currentX,currentY,'Q',0x7FFF);
      break;
    case 0x2D:
      putChar(currentX,currentY,'R',0x7FFF);
      break;
    case 0x1B:
      putChar(currentX,currentY,'S',0x7FFF);
      break;
    case 0x2C:
      putChar(currentX,currentY,'T',0x7FFF);
      break;
    case 0x3C:
      putChar(currentX,currentY,'U',0x7FFF);
      break;
    case 0x2A:
      putChar(currentX,currentY,'V',0x7FFF);
      break;
    case 0x1D:
      putChar(currentX,currentY,'W',0x7FFF);
      break;
    case 0x22:
      putChar(currentX,currentY,'X',0x7FFF);
      break;
    case 0x35:
      putChar(currentX,currentY,'Y',0x7FFF);
      break;
    case 0x1A:
      putChar(currentX,currentY,'Z',0x7FFF);
      break;
    case 0x55:
      if(shift_key) {
        putChar(currentX,currentY,'+',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'=',0x7FFF);
      }
      break;
    case 0x54:
      if(shift_key) {
        putChar(currentX,currentY,'{',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'[',0x7FFF);
      }
      break;
    case 0x61:
      if(shift_key) {
        putChar(currentX,currentY,'|',0x7FFF);
      }
      else {
        putChar(currentX,currentY,'\\',0x7FFF);
      }
      break;
    case 0x5B:
      if(shift_key) {
        putChar(currentX,currentY,'}',0x7FFF);
      }
      else {
        putChar(currentX,currentY,']',0x7FFF);
      }
      break;
    case 0x0E:
      if(shift_key) {
        putChar(currentX,currentY,(unsigned char) 170,0x7FFF);
      }
      else {
        putChar(currentX,currentY,'`',0x7FFF);
      }                
    default:
      break;
  }
}

void hilevel_handler_rst(ctx_t* ctx) {
  PL011_putc(UART0, 'R', true);
  
  initialiseProcTab();
  initFd();
  initPipes();
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
 *   processor via the IRQ interrupt signal,
 * - configuring then enabling PS/2 controllers st. an interrupt is
*    raised every time a byte is subsequently received, then
 * - enabling IRQ interrupts.
 */

  UART1->IMSC       |= 0x00000010; // enable UART    (Rx) interrupt
  UART1->CR          = 0x00000301; // enable UART (Tx+Rx)
 
  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

  // Configure the LCD display into 800x600 SVGA @ 36MHz resolution.

  SYSCONF->CLCD      = 0x2CAC;     // per per Table 4.3 of datasheet
  LCD->LCDTiming0    = 0x1313A4C4; // per per Table 4.3 of datasheet
  LCD->LCDTiming1    = 0x0505F657; // per per Table 4.3 of datasheet
  LCD->LCDTiming2    = 0x071F1800; // per per Table 4.3 of datasheet

  LCD->LCDUPBASE     = ( uint32_t )( &fb );

  LCD->LCDControl    = 0x00000020; // select TFT   display type
  LCD->LCDControl   |= 0x00000008; // select 16BPP display mode
  LCD->LCDControl   |= 0x00000800; // power-on LCD controller
  LCD->LCDControl   |= 0x00000001; // enable   LCD controller

  PS20->CR           = 0x00000010; // enable PS/2    (Rx) interrupt
  PS20->CR          |= 0x00000004; // enable PS/2 (Tx+Rx)
  PS21->CR           = 0x00000010; // enable PS/2    (Rx) interrupt
  PS21->CR          |= 0x00000004; // enable PS/2 (Tx+Rx)

  uint8_t ack;

        PL050_putc( PS20, 0xF4 );  // transmit PS/2 enable command
  ack = PL050_getc( PS20       );  // receive  PS/2 acknowledgement
        PL050_putc( PS21, 0xF4 );  // transmit PS/2 enable command
  ack = PL050_getc( PS21       );  // receive  PS/2 acknowledgement

  GICC0->PMR          = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00303010; // enable UART0/1 (Rx) interrupt & enable timer interrupt & enable PS2 interrupts
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor

  int_enable_irq();

  // Write example red/green/blue test pattern into the frame buffer.

  initDisplay();

  char_set['\0'] = "0000000000000000000000000000000000000000000000000000000000000000";

  char_set['!'] = "0010000000100000001000000010000000100000001000000000000000100000";
  char_set['\"'] = "0101000001010000000000000000000000000000000000000000000000000000";
  char_set['#'] = "0101000001010000111110000101000001010000111110000101000001010000";
  char_set['$'] = "0010000001110000101010000110000000110000101010000111000000100000";
  char_set['%'] = "1100100011010000000100000010000000100000010000000101100010011000";
  char_set['&'] = "0110000010010000100100001010000001000000101010001001000001101000";
  char_set['\''] = "0010000000100000000000000000000000000000000000000000000000000000";
  char_set['('] = "0010000001000000010000000100000001000000010000000100000000100000";
  char_set[')'] = "0100000000100000001000000010000000100000001000000010000001000000";
  char_set['*'] = "0101000000100000010100000000000000000000000000000000000000000000";
  char_set['+'] = "0000000000100000001000001111100000100000001000000000000000000000";
  char_set[','] = "0000000000000000000000000000000001100000011000000010000001000000";
  char_set['-'] = "0000000000000000000000000111000000000000000000000000000000000000";
  char_set['.'] = "0000000000000000000000000000000000000000000000000110000001100000";
  char_set['/'] = "0000100000010000000100000010000000100000010000000100000010000000";
  char_set['0'] = "0111000010001000100110001010100011001000100010001000100001110000";
  char_set['1'] = "0010000001100000001000000010000000100000001000000010000001110000";
  char_set['2'] = "0111000010001000000010000001000000100000010000001000000011111000";
  char_set['3'] = "0111000010001000000010000011000000001000000010001000100001110000";
  char_set['4'] = "0001000000110000010100000101000010010000111110000001000000010000";
  char_set['5'] = "1111100010000000100000001111000000001000000010001000100001110000";
  char_set['6'] = "0111000010001000100000001111000010001000100010001000100001110000";
  char_set['7'] = "1111100000001000000100000010000001000000010000000100000001000000";
  char_set['8'] = "0111000010001000100010000111000010001000100010001000100001110000";
  char_set['9'] = "0111000010001000100010001000100001111000000010000000100001110000";
  char_set[':'] = "0110000001100000000000000000000001100000011000000000000000000000";
  char_set[';'] = "0110000001100000000000000000000001100000011000000010000001000000";
  char_set['<'] = "0001000000100000010000001000000001000000001000000001000000000000";
  char_set['='] = "0000000000000000111110000000000011111000000000000000000000000000";
  char_set['>'] = "0100000000100000000100000000100000010000001000000100000000000000";
  char_set['?'] = "0111000010001000000010000001000000100000001000000000000000100000";
  char_set['@'] = "0111000010001000101110001010100010101000101110001000000001110000";
  char_set['A'] = "0111000010001000100010001111100010001000100010001000100010001000"; //A
  char_set['B'] = "1111000010001000100010001111000010001000100010001000100011110000"; //B
  char_set['C'] = "0111000010001000100000001000000010000000100000001000100001110000"; //C
  char_set['D'] = "1111000010001000100010001000100010001000100010001000100011110000"; //D
  char_set['E'] = "1111100010000000100000001111000010000000100000001000000011111000"; //E
  char_set['F'] = "1111100010000000100000001111000010000000100000001000000010000000"; //F
  char_set['G'] = "0111000010001000100000001000000010011000100010001000100001110000"; //G
  char_set['H'] = "1000100010001000100010001111100010001000100010001000100010001000"; //H
  char_set['I'] = "0111000000100000001000000010000000100000001000000010000001110000"; //I
  char_set['J'] = "0000100000001000000010000000100000001000000010001000100001110000"; //J
  char_set['K'] = "1000100010001000100100001110000010010000100010001000100010001000"; //K
  char_set['L'] = "1000000010000000100000001000000010000000100000001000000011111000"; //L
  char_set['M'] = "1000100011011000101010001000100010001000100010001000100010001000"; //M
  char_set['N'] = "1000100010001000100010001100100010101000100110001000100010001000"; //N
  char_set['O'] = "0111000010001000100010001000100010001000100010001000100001110000"; //O
  char_set['P'] = "1111000010001000100010001111000010000000100000001000000010000000"; //P
  char_set['Q'] = "0111000010001000100010001000100010001000101010001001000001101000"; //Q
  char_set['R'] = "1111000010001000100010001111000010001000100010001000100010001000"; //R
  char_set['S'] = "0111100010000000100000000111000000001000000010000000100011110000"; //S
  char_set['T'] = "1111100000100000001000000010000000100000001000000010000000100000"; //T
  char_set['U'] = "1000100010001000100010001000100010001000100010001000100001110000"; //U
  char_set['V'] = "1000100010001000100010001000100010001000100010000101000000100000"; //V
  char_set['W'] = "1000100010001000100010001000100010001000101010001101100010001000"; //W
  char_set['X'] = "1000100010001000010100000010000001010000100010001000100010001000"; //X
  char_set['Y'] = "1000100010001000100010000101000000100000001000000010000000100000"; //Y
  char_set['Z'] = "1111100000001000000100000010000001000000100000001000000011111000"; //Z
  char_set['['] = "0110000001000000010000000100000001000000010000000100000001100000";
  char_set['\\'] = "1000000001000000010000000010000000100000000100000001000000001000";
  char_set[']'] = "0110000000100000001000000010000000100000001000000010000001100000";
  char_set['^'] = "0010000001010000100010000000000000000000000000000000000000000000";
  char_set['_'] = "0000000000000000000000000000000000000000000000000000000011111000";
  char_set['`'] = "0010000000100000000000000000000000000000000000000000000000000000";  //ascii 96 Grave accent replaced with ascii 39 Apostrophe
  //ascii 97 - 122  lower case section
  char_set['{'] = "0010000001000000010000001000000001000000010000000100000000100000";
  char_set['|'] = "0010000000100000001000000010000000100000001000000010000000100000";
  char_set['}'] = "0010000000010000000100000000100000010000000100000001000000100000";
  char_set['~'] = "0000000000000000010000001010100000010000000000000000000000000000";

  //extra
  char_set[156] = "0011100001000000010000001111000001000000010000000100000011111000"; //ascii 156 Pound sign
  char_set[170] = "0000000000000000111110000000100000001000000000000000000000000000"; //ascii 170 Logical negation symbol

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
  else if( id == GIC_SOURCE_UART1 ) {
    UART1->ICR = 0x10;
  }
  else if     ( id == GIC_SOURCE_PS20 ) {
    
    uint8_t x = PL050_getc( PS20 );

    PL011_putc( UART0, '0',                      true );  
    PL011_putc( UART0, '<',                      true ); 
    PL011_putc( UART0, itox( ( x >> 4 ) & 0xF ), true ); 
    PL011_putc( UART0, itox( ( x >> 0 ) & 0xF ), true ); 
    PL011_putc( UART0, '>',                      true );

    if(prev_ps20_id != 0xF0) {
      keyboard_behaviour_0(x);
    }
    prev_ps20_id = x;
    
  }
  else if( id == GIC_SOURCE_PS21 ) {
    uint8_t x = PL050_getc( PS21 );

    PL011_putc( UART0, '1',                      true );  
    PL011_putc( UART0, '<',                      true ); 
    PL011_putc( UART0, itox( ( x >> 4 ) & 0xF ), true ); 
    PL011_putc( UART0, itox( ( x >> 0 ) & 0xF ), true ); 
    PL011_putc( UART0, '>',                      true ); 
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
      hilevel_yield(ctx);
      break;
    }

    case SYS_WRITE : { // 0x01 => write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );
      hilevel_write(ctx, fd, x, n);
      break;
    }

    case SYS_READ : { // 0x02 => read( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );
      hilevel_read(ctx, fd, x, n);
      break;
    }

    case SYS_FORK : { //0x03 => fork
      hilevel_fork(ctx);
      PL011_putc( UART0, 'F', true );
      break;
    }

    case SYS_EXIT : { //0x04 => exit( x )
      int x = (int) ctx->gpr[ 0 ];
      hilevel_exit(ctx, x);
      PL011_putc( UART0, 'E', true );
      break;
    }

    case SYS_EXEC : { //0x05 => exec( x )
      void *x = (void *) ctx->gpr[ 0 ];
      hilevel_exec(ctx, x);
      PL011_putc( UART0, 'X', true );
      break;
    }

    case SYS_KILL : { //0x06 => kill( pid, x )
      int pid = (int) ctx->gpr[ 0 ];
      int x   = (int) ctx->gpr[ 1 ];
      hilevel_kill(ctx, pid, x);
      PL011_putc( UART0, 'K', true );
      break;
    }

    case SYS_NICE : { //0x07 => nice( pid, x )
      int pid = (int) ctx->gpr[ 0 ];
      int32_t x   = (int32_t) ctx->gpr[ 1 ];
      hilevel_nice(pid, x);
      PL011_putc( UART0, 'N', true);
      break;
    }

    case SYS_PIPE : { //0x08 => pipe( fd )
      hilevel_pipe(ctx);
      PL011_putc( UART0, 'P', true);
      break;
    }

    case SYS_CLOSE : { //0x09 => close( fd )
      int fdIndex = (uint32_t) ctx->gpr[ 0 ];
      hilevel_close(ctx, fdIndex);
      PL011_putc( UART0, 'C', true );
      break;
    }

    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}
