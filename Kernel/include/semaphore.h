#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

/*
 * Interfaz de semáforos (sin busy waiting para el waiter).
 * Implementaciones en Kernel/sync/semaphore.c.
 *
 * sem_open con name == NULL crea un semáforo anónimo nuevo.
 * sem_open con name != NULL busca por nombre; si no existe, lo crea.
 * El refcount del slot crece con cada open y baja con cada close.
 */

int sem_open(const char *name, int initial_value);
int sem_wait(int id);
int sem_post(int id);
int sem_close(int id);

#endif
