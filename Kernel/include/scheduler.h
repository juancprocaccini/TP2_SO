#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "process.h"

/*
 * Interfaz del scheduler (Round Robin con prioridades).
 * Implementaciones en Kernel/processes/scheduler.c.
 *
 * El context switch ocurre en _irq00Handler (timer) y en el handler de
 * SYS_YIELD: scheduler_tick() / scheduler_pick_next() actualizan la variable
 * global next_rsp (en assembly), que el handler carga antes de iretq.
 */

void   scheduler_init(void);
void   scheduler_add(pid_t pid);
void   scheduler_remove(pid_t pid);
void   scheduler_tick(void);          /* llamado desde el IRQ 0 (timer) */
void   scheduler_block(pid_t pid);
void   scheduler_unblock(pid_t pid);
pid_t  scheduler_current(void);

#endif
