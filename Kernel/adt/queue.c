#include <queue.h>
#include <mem.h>
#include <stddef.h>

typedef struct queue_node {
    void *elem;
    struct queue_node *next;
} queue_node_t;

struct queue_t {
    queue_node_t *head;
    queue_node_t *tail;
};

queue_t *queue_new(void) {
    queue_t *q = (queue_t *)mem_alloc(sizeof(queue_t));
    if (!q) return NULL;
    q->head = NULL;
    q->tail = NULL;
    return q;
}

int queue_is_empty(queue_t *q) {
    return q == NULL || q->head == NULL;
}

int queue_enqueue(queue_t *q, void *elem) {
    if (!q) return -1;
    queue_node_t *n = (queue_node_t *)mem_alloc(sizeof(queue_node_t));
    if (!n) return -1;
    n->elem = elem;
    n->next = NULL;

    if (q->tail == NULL) {
        q->head = n;
        q->tail = n;
    } else {
        q->tail->next = n;
        q->tail = n;
    }
    return 0;
}

void *queue_dequeue(queue_t *q) {
    if (queue_is_empty(q)) return NULL;
    queue_node_t *n = q->head;
    void *elem = n->elem;
    q->head = n->next;
    if (q->head == NULL) q->tail = NULL;
    mem_free(n);
    return elem;
}

int queue_remove(queue_t *q, void *elem) {
    if (queue_is_empty(q)) return -1;
    queue_node_t *curr = q->head;
    queue_node_t *prev = NULL;

    while (curr != NULL) {
        if (curr->elem == elem) {
            if (prev == NULL) {
                q->head = curr->next;
            } else {
                prev->next = curr->next;
            }
            if (q->tail == curr) {
                q->tail = prev;
            }
            mem_free(curr);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;
}

void queue_free(queue_t *q) {
    if (!q) return;
    while (!queue_is_empty(q)) {
        queue_dequeue(q);
    }
    mem_free(q);
}