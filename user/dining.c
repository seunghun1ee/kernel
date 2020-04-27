#include "dining.h"
#include "P5.h"

sem_t mutex[ph_number];  //chopsticks
spoon_t spoon[ph_number];

void main_dining() {

    for(int i = 0; i < ph_number; i++) {
        sem_init(&mutex[i], 1);
        spoon[i].mutex_address = &mutex[i];
        spoon[i].left = false;
        spoon[i].right = false;
    }

    philosopher_t ph;
    ph.state = THINKING;

    for(int e=0; e < 4; e++){
        fork();
    }

    for(int j = 0; j < ph_number; j++) {
        if(!spoon[j].left) {  //when this mutex is free as left spoon
            ph.left_address = spoon[j].mutex_address;
            spoon[j].left = true;
            ph.right_address = spoon[(j+1)% ph_number].mutex_address;
            spoon[(j+1)% ph_number].right = true;
            break;
        }
    }

    while(1) {
        uint32_t lo = 1 << 1;
        uint32_t hi = 1 << 16;

        ph.state = THINKING;
        for(int y=0; y < 5; y++) {
            for( uint32_t x = lo; x < hi; x++ ) {
                int r = is_prime( x );            
            }
        }//THINKING time = 5 is_prime time

        ph.state = HUNGRY;
        sem_wait(ph.left_address);
        write(STDOUT_FILENO, "pick up left ", 15);
        // for(int y=0; y < 4; y++) {
        //     for( uint32_t x = lo; x < hi; x++ ) {
        //         int r = is_prime( x );            
        //     }
        // }//picking time = 4 is_prime time
        
        sem_wait(ph.right_address);
        write(STDOUT_FILENO, "pick up right ", 15);
        
        ph.state = EATING;
        write(STDOUT_FILENO, "eating ", 8);
        for(int y=0; y < 10; y++) {
            for( uint32_t x = lo; x < hi; x++ ) {
                int r = is_prime( x );   
            }  
        }//eating time = 10 is_prime time
        
        write(STDOUT_FILENO, "done ", 6);
        
    
        sem_post(ph.right_address);
        write(STDOUT_FILENO, "drop right ", 12);
        for(int y=0; y < 4; y++) {
            for( uint32_t x = lo; x < hi; x++ ) {
                int r = is_prime( x );            
            }
        }//dropping time = 4 is_prime time

        sem_post(ph.left_address);
        write(STDOUT_FILENO, "drop left ", 11);
        //yield();
        
    }

    


    exit(EXIT_SUCCESS);
}