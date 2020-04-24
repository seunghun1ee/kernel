#include "dining.h"
#include "P5.h"

sem_t mutex[ph_number];  //forks
spoon_t spoon[ph_number];

void main_dining() {

    for(int i = 0; i < ph_number; i++) {
        sem_init(&mutex[i], 1);
        spoon[i].mutex_address = &mutex[i];
        spoon[i].left = false;
        spoon[i].right = false;
    }

    philosopher_t ph;
    
    fork();
    fork();
    fork();

    for(int j = 0; j < ph_number; j++) {
        if(!spoon[j].left) {  //when this mutex is free as left spoon
            ph.left_address = spoon[j].mutex_address;
            spoon[j].left = true;
            ph.right_address = spoon[(j+1)%4].mutex_address;
            spoon[(j+1)%4].right = true;
            break;
        }
    }

    while(1) {
        uint32_t lo = 1 << 1;
        uint32_t hi = 1 << 16;

        sem_wait(ph.left_address);
        write(STDOUT_FILENO, "pick up left ", 14);
        for( uint32_t x = lo; x < hi; x++ ) {
            int r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );             
        }  //picking time = 4 is_prime time
        
        sem_wait(ph.right_address);
        write(STDOUT_FILENO, "pick up right ", 15);
        for( uint32_t x = lo; x < hi; x++ ) {
            int r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );             
        }
        
        write(STDOUT_FILENO, "eating ", 8);
        for( uint32_t x = lo; x < hi; x++ ) {
            int r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );   
        }  //eating time = 10 is_prime time
        write(STDOUT_FILENO, "done ", 6);
        
    
        sem_post(ph.right_address);
        write(STDOUT_FILENO, "drop right ", 12);
        for( uint32_t x = lo; x < hi; x++ ) {
            int r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );             
        }  //dropping time = 6 is_prime time

        sem_post(ph.left_address);
        write(STDOUT_FILENO, "drop left ", 11);
        for( uint32_t x = lo; x < hi; x++ ) {
            int r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );
            r = is_prime( x );                 
        }
        yield();
        
    }

    


    exit(EXIT_SUCCESS);
}