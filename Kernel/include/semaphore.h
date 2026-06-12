#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

// Tienen "k" antes para no confundir a gcc con funciones de GNU

void ksem_init(void); // Inicializa el array de semáforos (se llama en el main del Kernel)
int ksem_open(int id, uint64_t initial);
int ksem_open_kernel_side(int id, uint64_t initial);
int ksem_open_get_id(uint64_t initial);
int ksem_wait(int id);
int ksem_post(int id);
int ksem_post_no_yield(int id);
int ksem_close(int id);
int ksem_remove_waiter(int id, void *pcb);

#endif