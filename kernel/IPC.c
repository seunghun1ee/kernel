#include "IPC.h"
#include <assert.h>
#include <stdio.h>

queue_t *newQueue(int sizeArg) {
    queue_t *queue = malloc(sizeof(queue_t));
    queue->size = sizeArg;
    queue->front = -1;
    queue->rear = -1;
    queue->itemCount = 0;

    return queue;
}

void enqueue(queue_t *queue, uint8_t item) {
    if(queue->itemCount < queue->size) {
        if(queue->itemCount == 0) {
            queue->queueArray[0] = item;
            queue->front = 0;
            queue->rear = 0;
        }
        else if(queue->rear == queue->size-1) {
            queue->queueArray[0] = item;
            queue->rear = 0;
        }
        else {
            queue->rear++;
            queue->queueArray[queue->rear] = item;
        }
        queue->itemCount++;
    }
    else {
        printf("Queue is full\n");
    }
}

uint8_t peek(queue_t *queue) {
    if(queue->itemCount > 0) {
        return queue->queueArray[queue->front];
    }
    else {
        printf("Cannot peek, queue is empty\n");
    }
}

uint8_t dequeue(queue_t *queue) {
    if(queue->itemCount > 0) {
        uint8_t item = peek(queue);
        queue->itemCount--;
        queue->front++;
        return item;
    }
    else
    {
        printf("Queue is empty\n");
    } 
}

pipe_t *mkfifo(int idArg, int size) {
    queue_t *queue = newQueue(size);
    pipe_t *pipe = malloc(sizeof(pipe_t));
    pipe->id = idArg;
    pipe->queue = queue;
    pipe->open = false;
    return pipe;
}

void testQueue() {
    //test size init
    queue_t *test = newQueue(5);
    assert(test->size = 5);

    //test enqueue
    enqueue(test, (uint8_t) 0);
    assert(test->queueArray[test->front] == 0);
    assert(test->queueArray[test->rear] == 0);
    assert(test->front == 0);
    assert(test->rear == 0); 
    assert(test->itemCount == 1);

    enqueue(test, (uint8_t) 1);
    assert(test->queueArray[test->front] == 0);
    assert(test->queueArray[test->rear] == 1);
    assert(test->front == 0);
    assert(test->rear == 1); 
    assert(test->itemCount == 2);

    enqueue(test, (uint8_t) 2);
    assert(test->queueArray[test->front] == 0);
    assert(test->queueArray[test->rear] == 2);
    assert(test->front == 0);
    assert(test->rear == 2); 
    assert(test->itemCount == 3);

    enqueue(test, (uint8_t) 3);
    assert(test->queueArray[test->front] == 0);
    assert(test->queueArray[test->rear] == 3);
    assert(test->front == 0);
    assert(test->rear == 3); 
    assert(test->itemCount == 4);

    enqueue(test, (uint8_t) 4);
    assert(test->queueArray[test->front] == 0);
    assert(test->queueArray[test->rear] == 4);
    assert(test->front == 0);
    assert(test->rear == 4); 
    assert(test->itemCount == 5);

    enqueue(test, (uint8_t) 5);
    assert(test->queueArray[test->front] == 0);
    assert(test->queueArray[test->rear] == 4);
    assert(test->front == 0);
    assert(test->rear == 4); 
    assert(test->itemCount == 5);

    //test peek
    assert(peek(test) == 0);

    //test dequeue
    assert(dequeue(test) == 0);
    assert(test->front = 1);
    assert(dequeue(test) == 1);
    assert(test->front = 2);
    assert(dequeue(test) == 2);
    assert(test->front = 3);
    assert(dequeue(test) == 3);
    assert(test->front = 4);
    assert(dequeue(test) == 4);
    assert(test->front = 5);
    assert(dequeue(test));
    assert(test->front = 5);

    //test mix
    enqueue(test, (uint8_t) 'a');
    enqueue(test, (uint8_t) 'b');
    assert(dequeue(test) == 'a');
    enqueue(test, (uint8_t) 'c');
    assert(peek(test) == 'b');
    assert(dequeue(test) == 'b');
    assert(peek(test) == 'c');
    assert(dequeue(test) == 'c');
    assert(peek(test));
    enqueue(test, (uint8_t) 'd');
    assert(dequeue(test) == 'd');
    assert(dequeue(test));

    free(test);
    printf("queue test passed\n");
}

void testPipe() {
    //test mkfifo
    pipe_t *testPipe = mkfifo(0, 5);
    assert(testPipe->open == false);
    assert(testPipe->queue->itemCount == 0);
    assert(testPipe->queue->size == 5);


    printf("pipe test passed\n");
}

int main() {
    testQueue();
    testPipe();
    printf("test all passed\n");
    return 0;
}
