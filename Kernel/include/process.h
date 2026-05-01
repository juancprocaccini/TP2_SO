#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "defs.h"

/*
 * Estructuras y funciones de procesos.
 * Implementaciones en Kernel/processes/.
 */

typedef int64_t pid_t;

typedef enum {
	PROC_RUNNING,
	PROC_READY,
	PROC_BLOCKED,
	PROC_ZOMBIE,
	PROC_FREE
} ProcessState;

typedef struct PCB {
	pid_t          pid;
	pid_t          ppid;
	ProcessState   state;
	int            priority;
	uint64_t       rsp;
	uint64_t       rbp;
	uint64_t       stack_base;
	uint64_t       stack_size;
	char           name[32];
	int            is_foreground;
	int            fds[MAX_FDS];
	int            exit_status;
	pid_t          waiter;
} PCB;

typedef void (*process_entry_t)(int argc, char **argv);

/*
 * Crea un proceso. argv se copia al stack del nuevo proceso, fds[2] son
 * los FDs iniciales para stdin/stdout (heredables del padre).
 * Retorna el PID asignado, o -1 en error.
 */
pid_t process_create(process_entry_t entry, int argc, char **argv,
                     int priority, int is_foreground, int fds[2],
                     const char *name);

void  process_exit(int status);
pid_t process_getpid(void);
void  process_yield(void);

int   process_kill(pid_t pid);
int   process_nice(pid_t pid, int priority);
int   process_block(pid_t pid);
int   process_unblock(pid_t pid);
int   process_waitpid(pid_t pid);

/*
 * Llena buf con un snapshot de los procesos vivos (hasta max).
 * Retorna la cantidad escrita.
 */
int   process_print_all(PCB *buf, int max);

PCB * process_get(pid_t pid);

#endif
