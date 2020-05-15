<1> List of my work
    
    1. kernel/hilevel.c
        global variables {
            proc_stack stack[ MAX_PROCS ]
            int readyPcbIndex[ MAX_PROCS ]
            uint32_t capn = MAX_PROCS
            pipe_t pipes[MAX_PIPES]
            file_descriptor_t fd[50]

            extern void     main_console()
            extern uint32_t tos_proc

            uint16_t fb[ 600 ][ 800 ]
            uint16_t currentX = 0
            uint16_t currentY = 0
            char **char_set
            uint8_t prev_ps20_id = 0x0
            bool shift_key = false
        }
        functions {
            updateCapnAndReadyIndex()
            schedule()

            initialiseProcTab()
            initFd()
            initPipes()
            initDisplay()

            setStack()
            getIndexOfProcTable()
            getIndexOfStackByTos()
            getIndexOfStackByPid()

            findAvailableProcTab()
            findAvaialbeTos()
            findAvailableFd()
            findAvailablePipe()

            lineFeed()
            backspace()
            updateXY()
            putChar()

            hilevel_yield()
            hilevel_write()
            hilevel_read()
            hilevel_fork()
            hilevel_exit()
            hilevel_exec()
            hilevel_kill()
            hilevel_nice()
            hilevel_bnice()
            hilevel_pipe()
            hilevel_close()

            keyboard_behaviour_0()
            
            
            *These three functions are modified from COMS20001 labsheet 3, 4 and 6 
            hilevel_handler_rst()
            hilevel_handler_irq()
            hilevel_handler_svc()

            *These two functions are influenced from https://www.codesdope.com/blog/article/making-a-queue-using-an-array-in-c/
            push()
            pop()
        }

    2. kernel/hilevel.h
        constants {
            STACK_SIZE = 0x00001000
            MAX_PIPES = 20
            QUEUE_LEN = 1000
        }
        type definitions {
            proc_stack
            pipe_t
            file_descriptor_t
            update_display_op
        }

    3. user/console.c
        No significant changes but
            1. adding variable "extern void main_dining();"
            2. adding if condition "else if( 0 == strcmp( x, "dining" ) ) { return &main_dining; }"
            3. puts( "console$ ", 7 ); -> puts( "console$ ", 9 );
        were done
    
    4. user/dining.c
        All codes
    
    5. user/dining.h
        All codes
    
    6. user/libc.c
        functions {
            bnice()
            pipe()
            close()
        }
    
    7. user/libc.h
        constants {
            SYS_BNICE = 0x08
            SYS_PIPE = 0x09
            SYS_CLOSE = 0x0A
        }
    
    8. image.ld
        Allocated new stack for processes above top of svc stack

    9. Every .pbm files in bitmap directory

Everything else is from COMS20001 lab sheet 3, 4 and 6

<2> How to start

    Open three terminals at the root of the directory
    For one terminal, launch QEMU with the command
        make launch-qemu
    For another terminal, connect to the console with the command
        nc 127.0.0.1 1235
    For the third terminal, launch gdb with the command
        make launch-gdb
    
    type "continue" in gdb to start the kernel

    The kernel has 4 programs P3, P4, P5 and dining

    To execute dining Philosophers, you need to write command in console
        execute dining

<3> How they work

    On reset interrupt request, lolevel_handler_rst is called and it calls hilevel_handler_rst()

    In hilevel_handler_rst(),
        1. PCBs, process-stack, file_descriptors and pipes are initialised
        2. After PCBs are initialised, execute the 0-th process which is the console
        3. PL011, PL050, PL111, SP804 and GIC interrupts are configured
        4. Enable IRQ interrupt
        5. initialise the display and load 8x8 bitmap character set
    
    In main_console(), console use gets() to get string command and tokenise them
    Then it executes the command
    On program execution, console calls fork() to clone current process then calls exec()
    On process termination, console calls kill() to the process specified by pid

    libc functions yield(), write(), read(), fork(), exit(), exec(), kill(), nice(), bnice(), pipe(), close()
    make system call in SVC mode with its system call identifier

    The SVC mode system call triggers lolevel_handler_svc and it calls hilevel_handler_svc()
    Based on the identifier, it performs hi-level kernel side functions for libc functions

    On every interrupt request, lolevel_handler_irq is triggered and it calls hilevel_handler_irq()

    In hilevel_handler_irq(), it handles the interrupt depends on the source of the request
        source: /  action:
        TIMER   /  calls schedule()
        UART1   /  handles console input/output
        PS20    /  handles PS/2 keyboard input
        PS21    /  handles PS/2 mouse input (*)
    
    The schedule() is priority-based + ageing process scheduler
    In schedule(),
        1. The scheduler resets the priority of the current process to base-priority
        2. Increments priority of every process that is in STATUS_READY
        3. Looks for the process with the highest priority and picks that as next_exec process
        4. Call dispatch() to execute next process

    For the detailed descriptions, please check the comments in functions

    (*) Not implemented yet

<4> How the display works
    
    global variable fb in hilevel.c represents 800 x 600 colour matrix
    on initDisplay(), it fills all elements in the matrix with black background colour
    On keyboard input, it calls putChar() to display the character of the input at the current coordinates
    putChar() looks for the bitmap of the character from char_set and updates 8x8 grid from the current coordinates
    
    When hilevel_write() is called, it also calls putChar() to display the same content of UART1

    The current coordinates, currentX and currentY are controlled by functions lineFeed(), backspace() and updateXY()

    That's all for now

<5> Future plan on extension

    Due to COVID-19, the coursework was disrupted and the extension was unfortunately not enough to finish the stage 3
    In the current stage, the display function is primitive and does not provide better user experience compared to console from UART1
    My plans to improve this are
    
    1.  Make the PL111 display to have console functionality

        On hilevel_write, send char to PS20->DATA by using PL050_putc
        Write console2.c based on console.c to get command from the display
        Create a buffer for the input from display in hilevel.c
        Instead of using PL011_getc(), read from the buffer to get command character data
        This will make users interact with the system just from the display
    
    2.  Use cursor related registers in PL111, to implement PS/2 mouse interaction

        Create 32x32 cursor images and store that in PL111->ClcdCrsrImage register in LBBP format as the PL111 manual specifies
        Use PL111->ClcdCrsrCtrl register to control the image of the cursor and enable/disable displaying the cursor
        On PS/2 mouse input, request interrupt by using PL111->ClcdCrsrIMSC register
        Get PS/2 mouse input from PS21->DATA and perform mouse position update by changing the value of PL111->ClcdCrsrXY register
        Clear interrupt at register PL111->ClcdCrsrICR
        This will make users have mouse interaction with the system

    3.  Design GUI

        Write some graphics displaying functions in hilevel.c
        Such as icons for Dining Philosophers program, notepad program, etc.
        Plus, better-designed terminal
        This will make users execute programs just from mouse clicking
    
    4.  Design graphics for the Dining Philosophers program

        This will make users see which chopsticks are picked or dropped easily