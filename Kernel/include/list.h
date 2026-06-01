#ifndef LIST_H
#define LIST_H

typedef int (*list_cmp)(void *a, void *b);
typedef struct list_t list_t;

list_t *list_new(list_cmp cmp);
void    list_free(list_t *l);
int     list_add(list_t *l, void *elem);
int     list_remove(list_t *l, void *elem);

void   *list_next(list_t *l);
int     list_is_empty(list_t *l);

#endif