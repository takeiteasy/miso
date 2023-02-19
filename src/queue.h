//
//  queue.h
//  colony
//
//  Created by George Watson on 19/02/2023.
//

#ifndef queue_h
#define queue_h
#include <pthread.h>
#include <stdlib.h>

typedef struct QueueBucket {
    void *data;
    struct QueueBucket *next;
} QueueBucket;

typedef struct {
    QueueBucket root, *tail;
    int count;
    pthread_mutex_t read, modify;
} Queue;

Queue NewQueue(void);
void QueueAdd(Queue *queue, void *data);
QueueBucket* QueuePop(Queue *queue);
void DestroyQueue(Queue *queue);

#endif /* queue_h */
