#ifndef __DINING_H
#define __DINING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc.h"


typedef struct {
    int index;
    sem_t left;
    sem_t right;
} philosopher_t;


#endif