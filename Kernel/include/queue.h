#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue_t queue_t;

queue_t *queue_new(void);
void     queue_free(queue_t *q);
int      queue_enqueue(queue_t *q, void *elem);
void    *queue_dequeue(queue_t *q);
int      queue_remove(queue_t *q, void *elem); 
int      queue_is_empty(queue_t *q);

#endif