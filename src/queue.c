//
//  queue.c
//  colony
//
//  Created by George Watson on 19/02/2023.
//

#include "queue.h"

Queue NewQueue(void) {
    Queue result = {
        .root = (QueueBucket) {
            .data = NULL,
            .next = NULL,
        },
        .tail = NULL,
        .count = 0
    };
    pthread_mutex_init(&result.read, NULL);
    pthread_mutex_lock(&result.read);
    pthread_mutex_init(&result.modify, NULL);
    return result;
}

void QueueAdd(Queue *queue, void *data) {
    QueueBucket *bucket = malloc(sizeof(QueueBucket));
    bucket->data = data;
    bucket->next = NULL;
    
    pthread_mutex_lock(&queue->modify);
    
    if (!queue->count)
        queue->root.next = queue->tail = bucket;
    else
        queue->tail = queue->tail->next = bucket;
    queue->count++;
    
    pthread_mutex_unlock(&queue->modify);
    pthread_mutex_unlock(&queue->read);
}

QueueBucket* QueuePop(Queue *queue) {
    QueueBucket *next = queue->root.next;
    if (!next)
        return NULL;
    queue->root.next = next->next;
    return next;
}

void DestroyQueue(Queue *queue) {
    QueueBucket *cursor = queue->root.next;
    while (cursor) {
        QueueBucket *next = cursor->next;
        free(cursor);
        cursor = next;
    }
    pthread_mutex_destroy(&queue->read);
    pthread_mutex_destroy(&queue->modify);
}
