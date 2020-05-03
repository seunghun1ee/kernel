#include "dining.h"
#include "P5.h"

int chopstick[ph_number][2];  //pipe fds
chopstick_t taken[ph_number];  //only for initialisation

void main_dining_pipe() {

    for(int i = 0; i < ph_number; i++) {
        pipe(chopstick[i]);
        taken[i].left = false;
        taken[i].right = false;
        write(chopstick[i][1], "a", 1);
    }

    philosopher2_t ph;
    ph.state = THINKING;

    for(int e=0; e < 4; e++){
        fork();
    }

    for(int j = 0; j < ph_number; j++) {
        if(!taken[j].left) {  //when this mutex is free as left spoon
            ph.left_read = chopstick[j][0];
            ph.left_write = chopstick[j][1];
            taken[j].left = true;
            ph.right_read = chopstick[(j+1)% ph_number][0];
            ph.right_write = chopstick[(j+1)% ph_number][1];
            taken[(j+1)% ph_number].right = true;
            break;
        }
    }

    while(1) {
        uint32_t lo = 1 << 1;
        uint32_t hi = 1 << 16;

        ph.state = THINKING;

        ph.state = HUNGRY;
        while(read(ph.left_read, "wait", 1) == 0) {
            //wait
        }
        write(STDOUT_FILENO, "pick up left ", 15);
        
        while(read(ph.right_read, "wait", 1) == 0) {
            //wait
        }
        write(STDOUT_FILENO, "pick up right ", 15);
        
        ph.state = EATING;
        write(STDOUT_FILENO, "eating ", 8);
        for(int y=0; y < 10; y++) {
            for( uint32_t x = lo; x < hi; x++ ) {
                int r = is_prime( x );   
            }  
        }//eating time = 10 is_prime time
        
        write(STDOUT_FILENO, "done ", 6);
        
        int right_err = write(ph.right_write, "a", 1);
        if(right_err == 0) {
            write(STDOUT_FILENO, "drop right error ",18);
            exit(EXIT_SUCCESS);
        }
        write(STDOUT_FILENO, "drop right ", 12);
        int left_err = write(ph.left_write, "a", 1);
        if(left_err == 0) {
            write(STDOUT_FILENO, "drop left error ",17);
            exit(EXIT_SUCCESS);
        }
        write(STDOUT_FILENO, "drop left ", 11);
        yield();
        
    }

    for(int i = 0; i < ph_number; i++) {
        close(chopstick[i][0]);
        close(chopstick[i][1]);
    }


    exit(EXIT_SUCCESS);
}