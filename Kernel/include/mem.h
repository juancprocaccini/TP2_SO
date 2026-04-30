#ifndef MEM_H
#define MEM_H

#include <stdint.h>

/*
 * Interfaz compartida para los memory managers (buddy system y propio).
 * Ambas implementaciones deben respetar estas firmas.
 * Implementaciones en Kernel/mem/.
 */

// void * mem_alloc(uint64_t size);
// void   mem_free(void * ptr);
// void   mem_state(uint64_t *total, uint64_t *used, uint64_t *free);

#endif
