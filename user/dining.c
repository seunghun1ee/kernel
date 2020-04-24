#include "dining.h"

sem_t mutex[4];  //forks

void main_dining() {

    for(int i = 0; i < 5; i++) {
        sem_init(&mutex[i], 1);
    }

    philosopher_t ph;
    
    int pid0 = fork();
    int pid1 = fork();

    if(pid0 == 0 && pid1 == 0) {
        ph.index = 0;
        ph.left = mutex[0];
        ph.right = mutex[1];

    }
    else if(pid0 != 0 && pid1 == 0) {
        ph.index = 1;
        ph.left = mutex[1];
        ph.right = mutex[2];
    }
    else if(pid0 == 0 && pid1 != 0) {
        ph.index = 2;
        ph.left = mutex[2];
        ph.right = mutex[3];
    }
    else if(pid0 != 0 && pid1 != 0) {
        ph.index = 3;
        ph.left = mutex[3];
        ph.right = mutex[0];
    }

    while(1) {
        sem_wait(&ph.left);
        write(STDOUT_FILENO, "pick up left", 13);
        yield();
        //sleep(2);
    
        sem_wait(&ph.right);
        write(STDOUT_FILENO, "pick up right", 14);
        write(STDOUT_FILENO, "eating", 7);
        yield();
    
        //sleep(4);  //eat
    
        sem_post(&ph.right);
        write(STDOUT_FILENO, "drop right", 11);

        sem_post(&ph.left);
        write(STDOUT_FILENO, "drop left", 10);
        yield();
    }

    


    exit(EXIT_SUCCESS);
}