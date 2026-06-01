#include <list.h>
#include <mem.h>
#include <stddef.h>

typedef struct list_node {
    void *elem;
    struct list_node *next;
    struct list_node *prev;
} list_node_t;

struct list_t {
    list_node_t *current;
    list_cmp cmp;
};

list_t *list_new(list_cmp cmp) {
    list_t *l = (list_t *)mem_alloc(sizeof(list_t));
    if (!l) return NULL;
    l->current = NULL;
    l->cmp = cmp;
    return l;
}

int list_is_empty(list_t *l) {
    return l == NULL || l->current == NULL;
}

int list_add(list_t *l, void *elem) {
    if (!l) return -1;
    list_node_t *n = (list_node_t *)mem_alloc(sizeof(list_node_t));
    if (!n) return -1;
    n->elem = elem;

    if (l->current == NULL) {
        n->next = n;
        n->prev = n;
        l->current = n;
    } else {
        list_node_t *tail = l->current->prev;
        tail->next = n;
        n->prev = tail;
        n->next = l->current;
        l->current->prev = n;
    }
    return 0;
}

int list_remove(list_t *l, void *elem) {
    if (list_is_empty(l)) return -1;
    list_node_t *start = l->current;
    list_node_t *curr = start;
    
    do {
        int match = l->cmp ? (l->cmp(curr->elem, elem) == 0) : (curr->elem == elem);
        if (match) {
            if (curr->next == curr) {
                l->current = NULL;
            } else {
                curr->prev->next = curr->next;
                curr->next->prev = curr->prev;
                if (l->current == curr) l->current = curr->next;
            }
            mem_free(curr);
            return 0;
        }
        curr = curr->next;
    } while (curr != start);
    
    return -1;
}

void *list_next(list_t *l) {
    if (list_is_empty(l)) return NULL;
    void *elem = l->current->elem;
    l->current = l->current->next;
    return elem;
}

void list_free(list_t *l) {
    if (!l) return;
    while (!list_is_empty(l)) {
        list_remove(l, l->current->elem);
    }
    mem_free(l);
}