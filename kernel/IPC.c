#include "hilevel.h"

queue_t *newQueue(int sizeArg) {
    queue_t *queue = malloc(sizeof(queue_t));
    queue->size = sizeArg;
    queue->front = -1;
    queue->rear = -1;
    queue->itemCount = 0;

    return queue;
}
