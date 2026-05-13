#ifndef MEM_USER_H
#define MEM_USER_H

#include <stdint.h>

/*
 * API interna del user heap — solo el syscall layer puede llamar estas funciones.
 * No incluir en mem.h para preservar la invariante de API pública del kernel.
 *
 * Implementadas en mem/mem_custom.c o mem/mem_buddy.c según compile target.
 */

void  user_mem_init(void *base, uint64_t size);
void *user_mem_alloc(uint64_t size);
void  user_mem_free(void *ptr);
void  user_mem_state(uint64_t *total, uint64_t *used, uint64_t *free_out);

#endif
