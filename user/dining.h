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
    int index;
    int state;
    int left_read;
    int left_write;
    int right_read;
    int right_write;
} philosopher_t;


typedef struct {
    bool left;
    bool right;
} chopstick_t;


#endif