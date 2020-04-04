#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <stdlib.h>

typedef struct {
  int size;  //size of the queue
  int front;  //index of the front item 
  int rear;  //index of the rear item
  int itemCount;  //number of items in queue
  uint8_t queueArray[1000];  //The array for queue (each element represent one byte)
} queue_t;

typedef struct {
  int   id;  //id of the pipe
  queue_t *queue;  //pointer to the queue object
} pipe_t;