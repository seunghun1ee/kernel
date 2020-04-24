#ifndef __DINING_H
#define __DINING_H

#define ph_number 8

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc.h"



typedef struct {
    int index;
    uint32_t left_address;
    uint32_t right_address;
} philosopher_t;

typedef struct {
    uint32_t mutex_address;
    bool left;
    bool right;
} spoon_t;


#endif