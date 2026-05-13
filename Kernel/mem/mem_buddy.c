#ifdef BUDDY_SYSTEM

#include <mem.h>
#include <mem_user.h>
#include <defs.h>
#include <stddef.h>

/*
 * Buddy system — orden mínimo 12 (4 KB), máximo 24 (16 MB).
 *
 * Cada heap es exactamente 2^24 bytes, por lo que mem_init lo registra como
 * un único bloque raíz de orden 24 — sin descomposición inicial.
 *
 * El encabezado de 8 bytes (HDR_SIZE) precede a cada bloque alocado:
 *   byte 0: order (uint8_t); bytes 1-7: padding.
 * Los bloques libres reutilizan esos 8 bytes como BuddyNode (16 bytes;
 * cabe porque MIN_ORDER=12 → 4 KB mínimo).
 *
 * La dirección del buddy se calcula con XOR relativo a heap_base:
 *   offset(block) XOR (1 << order)
 * Funciona porque cada bloque está alineado a su propio tamaño.
 */

#define MIN_ORDER  12
#define MAX_ORDER  24
#define NUM_ORDERS (MAX_ORDER - MIN_ORDER + 1)
#define HDR_SIZE   8

typedef struct BuddyNode {
    struct BuddyNode *prev;
    struct BuddyNode *next;
} BuddyNode;

typedef struct {
    BuddyNode free_list[NUM_ORDERS];   /* cabezas sentinela por orden */
    void     *heap_base;
    uint64_t  heap_sz;
    uint64_t  used_bytes;
} BuddyHeap;

static BuddyHeap kernel_heap;
static BuddyHeap user_heap;

/* ---- listas libres ---- */

static void fl_insert(BuddyHeap *h, int order, void *block) {
    BuddyNode *node = (BuddyNode *)block;
    BuddyNode *head = &h->free_list[order - MIN_ORDER];
    node->prev = head;
    node->next = head->next;
    if (head->next) head->next->prev = node;
    head->next = node;
}

static void fl_remove(BuddyNode *node) {
    node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
}

static int fl_empty(BuddyHeap *h, int order) {
    return h->free_list[order - MIN_ORDER].next == NULL;
}

static void *fl_pop(BuddyHeap *h, int order) {
    BuddyNode *node = h->free_list[order - MIN_ORDER].next;
    fl_remove(node);
    return (void *)node;
}

/* ---- operaciones de heap ---- */

static void heap_init(BuddyHeap *h, void *base, uint64_t size) {
    h->heap_base  = base;
    h->heap_sz    = size;
    h->used_bytes = 0;

    for (int i = 0; i < NUM_ORDERS; i++)
        h->free_list[i].prev = h->free_list[i].next = NULL;

    /* Descomposición binaria: inserta bloques libres potencia-de-dos */
    uint64_t offset = 0;
    for (int o = MAX_ORDER; o >= MIN_ORDER; o--) {
        uint64_t chunk = (uint64_t)1 << o;
        if (offset + chunk <= size) {
            fl_insert(h, o, (void *)((uint64_t)base + offset));
            offset += chunk;
        }
    }
}

static void *heap_alloc(BuddyHeap *h, uint64_t size) {
    if (!h->heap_base || size == 0)
        return NULL;

    uint64_t need = size + HDR_SIZE;

    int order = MIN_ORDER;
    while (order <= MAX_ORDER && ((uint64_t)1 << order) < need)
        order++;
    if (order > MAX_ORDER)
        return NULL;

    int found = -1;
    for (int o = order; o <= MAX_ORDER; o++) {
        if (!fl_empty(h, o)) { found = o; break; }
    }
    if (found < 0)
        return NULL;

    void *block = fl_pop(h, found);

    while (found > order) {
        found--;
        void *buddy = (void *)((uint64_t)block + ((uint64_t)1 << found));
        fl_insert(h, found, buddy);
    }

    *((uint8_t *)block) = (uint8_t)order;
    h->used_bytes += (uint64_t)1 << order;
    return (uint8_t *)block + HDR_SIZE;
}

static void heap_free(BuddyHeap *h, void *ptr) {
    if (!ptr)
        return;

    void *block = (uint8_t *)ptr - HDR_SIZE;
    int   order = (int)*((uint8_t *)block);
    h->used_bytes -= (uint64_t)1 << order;

    while (order < MAX_ORDER) {
        /* Buddy relativo a heap_base, usando XOR sobre el offset */
        uint64_t offset = (uint64_t)block - (uint64_t)h->heap_base;
        uint64_t buddy_off = offset ^ ((uint64_t)1 << order);
        void *buddy = (void *)((uint64_t)h->heap_base + buddy_off);

        if (buddy_off + ((uint64_t)1 << order) > h->heap_sz)
            break;

        int buddy_free = 0;
        BuddyNode *n = h->free_list[order - MIN_ORDER].next;
        while (n) {
            if ((void *)n == buddy) { buddy_free = 1; break; }
            n = n->next;
        }
        if (!buddy_free) break;

        fl_remove((BuddyNode *)buddy);
        if (buddy < block) block = buddy;
        order++;
    }

    fl_insert(h, order, block);
}

static void heap_state(BuddyHeap *h, uint64_t *total, uint64_t *used, uint64_t *free_out) {
    if (total)    *total    = h->heap_sz;
    if (used)     *used     = h->used_bytes;
    if (free_out) *free_out = h->heap_sz - h->used_bytes;
}

/* ---- Kernel heap — API pública (mem.h) ---- */

void  mem_init(void *base, uint64_t size)                       { heap_init(&kernel_heap, base, size); }
void *mem_alloc(uint64_t size)                                  { return heap_alloc(&kernel_heap, size); }
void  mem_free(void *ptr)                                       { heap_free(&kernel_heap, ptr); }
void  mem_state(uint64_t *t, uint64_t *u, uint64_t *f)         { heap_state(&kernel_heap, t, u, f); }

/* ---- User heap — API interna (mem_user.h) ---- */

void  user_mem_init(void *base, uint64_t size)                  { heap_init(&user_heap, base, size); }
void *user_mem_alloc(uint64_t size)                             { return heap_alloc(&user_heap, size); }
void  user_mem_free(void *ptr)                                  { heap_free(&user_heap, ptr); }
void  user_mem_state(uint64_t *t, uint64_t *u, uint64_t *f)    { heap_state(&user_heap, t, u, f); }

#endif /* BUDDY_SYSTEM */
