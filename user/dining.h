#ifndef __DINING_H
#define __DINING_H

#define ph_number 16
#define THINKING 0
#define HUNGRY 1
#define EATING 2

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc.h"


typedef struct {
    int index;  //index of philosopher
    int state;  //state of philosopher
    int left_read;  //fd of read-end of left pipe (chopstick)
    int left_write;  //fd of write-end of left pipe (chopstick)
    int right_read;  //fd of read-end of right pipe (chopstick)
    int right_write;  //fd of write-end of right pipe (chopstick)
} philosopher_t;


typedef struct {
    bool left;  //boolean field to check if this chopstick is taken as left chopstick
    bool right; //boolean field to check if this chopstick is taken as right chopstick
} chopstick_t;


#endif