#ifndef BUDDY_SYSTEM

#include <mem.h>
#include <mem_user.h>
#include <stddef.h>

/*
 * First-fit con free list implícita.
 *
 * El estado de cada heap vive en un CustomHeap (dos instancias estáticas:
 * kernel_heap y user_heap). Los helpers privados reciben un puntero a la
 * instancia y no tocan estado global.
 *
 * Splitting: si el remanente >= sizeof(header) + 8 bytes.
 * Coalescing: inmediato con vecino siguiente y luego con anterior.
 * Payloads alineados a múltiplos de 8.
 */

#define ALIGN8(x)   (((x) + 7UL) & ~7UL)
#define MIN_SPLIT   (sizeof(BlockHeader) + 8UL)

typedef struct BlockHeader {
    uint64_t            size;   /* payload en bytes, sin contar el header */
    struct BlockHeader *next;
    int                 free;
} BlockHeader;

typedef struct {
    BlockHeader *head;
    uint64_t     total_size;
    uint64_t     used_bytes;
} CustomHeap;

static CustomHeap kernel_heap;
static CustomHeap user_heap;

static void heap_init(CustomHeap *h, void *base, uint64_t size) {
    BlockHeader *bh = (BlockHeader *)base;
    bh->size      = size - sizeof(BlockHeader);
    bh->next      = NULL;
    bh->free      = 1;
    h->head       = bh;
    h->total_size = size;
    h->used_bytes = 0;
}

static void *heap_alloc(CustomHeap *h, uint64_t size) {
    if (!h->head || size == 0)
        return NULL;

    size = ALIGN8(size);

    BlockHeader *cur = h->head;
    while (cur) {
        if (cur->free && cur->size >= size) {
            if (cur->size >= size + MIN_SPLIT) {
                BlockHeader *split = (BlockHeader *)((uint8_t *)cur + sizeof(BlockHeader) + size);
                split->size = cur->size - size - sizeof(BlockHeader);
                split->next = cur->next;
                split->free = 1;
                cur->size   = size;
                cur->next   = split;
            }
            cur->free      = 0;
            h->used_bytes += sizeof(BlockHeader) + cur->size;
            return (void *)((uint8_t *)cur + sizeof(BlockHeader));
        }
        cur = cur->next;
    }
    return NULL;
}

static void heap_free(CustomHeap *h, void *ptr) {
    if (!ptr)
        return;

    BlockHeader *blk = (BlockHeader *)((uint8_t *)ptr - sizeof(BlockHeader));
    blk->free      = 1;
    h->used_bytes -= sizeof(BlockHeader) + blk->size;

    if (blk->next && blk->next->free) {
        blk->size += sizeof(BlockHeader) + blk->next->size;
        blk->next  = blk->next->next;
    }

    BlockHeader *prev = NULL;
    BlockHeader *cur  = h->head;
    while (cur && cur != blk) {
        prev = cur;
        cur  = cur->next;
    }
    if (prev && prev->free) {
        prev->size += sizeof(BlockHeader) + blk->size;
        prev->next  = blk->next;
    }
}

static void heap_state(CustomHeap *h, uint64_t *total, uint64_t *used, uint64_t *free_out) {
    if (total)    *total    = h->total_size;
    if (used)     *used     = h->used_bytes;
    if (free_out) *free_out = h->total_size - h->used_bytes;
}

/* ---- Kernel heap — API pública (mem.h) ---- */

void mem_init(void *base, uint64_t size)                        { heap_init(&kernel_heap, base, size); }
void *mem_alloc(uint64_t size)                                  { return heap_alloc(&kernel_heap, size); }
void  mem_free(void *ptr)                                       { heap_free(&kernel_heap, ptr); }
void  mem_state(uint64_t *t, uint64_t *u, uint64_t *f)         { heap_state(&kernel_heap, t, u, f); }

/* ---- User heap — API interna (mem_user.h) ---- */

void  user_mem_init(void *base, uint64_t size)                  { heap_init(&user_heap, base, size); }
void *user_mem_alloc(uint64_t size)                             { return heap_alloc(&user_heap, size); }
void  user_mem_free(void *ptr)                                  { heap_free(&user_heap, ptr); }
void  user_mem_state(uint64_t *t, uint64_t *u, uint64_t *f)    { heap_state(&user_heap, t, u, f); }

#endif /* !BUDDY_SYSTEM */
