#ifndef MEM_H
#define MEM_H

#include <stdint.h>

/*
 * Interfaz compartida para los memory managers (buddy system y propio).
 * Ambas implementaciones (mem/mem_custom.c y mem/mem_buddy.c) deben respetar
 * estas firmas — el switch entre managers se hace en compile time con
 * -DBUDDY_SYSTEM (ver Makefile raíz, target `buddy`).
 *
 * Implementaciones en Kernel/mem/.
 */

void   mem_init(void *base, uint64_t size);
void * mem_alloc(uint64_t size);
void   mem_free(void *ptr);
void   mem_state(uint64_t *total, uint64_t *used, uint64_t *free);

#endif
