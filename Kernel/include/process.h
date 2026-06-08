#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "defs.h"

typedef int pid_t;

typedef enum
{
    LOW = 0,
    MEDIUM,
    HIGH
} priority_t;
typedef enum
{
    FREE = 0,
    READY,
    BLOCKED,
    ZOMBIE
} pstate_t;

typedef struct PCB
{
    pid_t pid;
    pstate_t state;
    priority_t priority;
    uint64_t rsp;
    uint64_t stack_base;
    int argc;
    char **argv;
    char name[32];

    /* 0=stdin, 1=stdout, 2=stderr. Valores >= 3 indican (pipe_id + 3) */
    int fds[3];

    int64_t exit_status;
    int killable;            /* idle = 0 */
    struct PCB *waiting_me;  /* Proceso que me hizo wait() */
    struct PCB *waiting_for; /* Proceso por el que estoy haciendo wait() */

    /* -1 si no está esperando un semáforo. Otro valor indica el ID del semáforo */
    int64_t blocked_by_sem;
} PCB;

typedef int (*entry_t)(char **argv, int argc);

/* --- Creación e infraestructura --- */
pid_t process_create(entry_t rip, priority_t pri, int killable,
                     char **argv, int argc, int fds[3]);
void process_wrapper(entry_t rip, char **argv, int argc, pid_t pid);
PCB *process_get(pid_t pid);

/* --- Ciclo de vida (F4) --- */

/*
 * Termina el proceso running con el código dado. Desplanifica, notifica
 * al waiter si existe, y fuerza context switch. No retorna.
 */
void process_exit(int status);

/*
 * Retorna el pid del proceso running.
 */
pid_t process_getpid(void);

/*
 * Cede voluntariamente el CPU (int 0x20 sintético).
 */
void process_yield(void);

/*
 * Espera a que pid termine. Copia el exit status en *ret_out si no es NULL.
 * Retorna pid en éxito, -1 en error (pid inválido, ya tiene waiter, o
 * el proceso fue killed antes de hacerse ZOMBIE).
 */
pid_t process_waitpid(pid_t pid, int *ret_out);

/*
 * Mata pid incondicionalmente (si killable). Libera recursos.
 * Si pid == running, fuerza context switch. Retorna 0 / -1.
 */
int process_kill(pid_t pid);

/*
 * Cambia la prioridad de pid. Retorna 0 / -1.
 */
int process_nice(pid_t pid, priority_t priority);

/*
 * Bloquea / desbloquea pid externamente. Retorna 0 / -1.
 */
int process_block(pid_t pid);
int process_unblock(pid_t pid);

/*
 * Retorna el pstate_t de pid, o -1 si pid inválido.
 */
int process_get_status(pid_t pid);

/* --- ps (F4) --- */

/*
 * Snapshot de un proceso para userland. Refleja exactamente la vista
 * que pide la consigna: nombre, pid, prioridad, estado, punteros de
 * stack, flag foreground, y descriptores de archivo.
 */
typedef struct
{
    pid_t pid;
    char name[32];
    priority_t priority;
    pstate_t state;
    uint64_t rsp;
    uint64_t stack_base;
    int foreground;
    int fds[3];
} ProcessInfo;

typedef struct
{
    ProcessInfo *entries;
    int count;
} ProcessInfoList;

/*
 * Aloca en el kernel heap una ProcessInfoList con todos los procesos
 * no-FREE. El caller libera vía process_free_ps().
 * Retorna NULL si no hay procesos o falla la alocación.
 */
ProcessInfoList *process_ps(void);
void process_free_ps(ProcessInfoList *list);

/*
 * Helper compartido por ps y el scheduler: un proceso es foreground
 * si la shell lo está esperando directamente.
 * (F6 lo extenderá para cubrir el writer de un pipe.)
 */
int is_foreground(PCB *p);

#endif /* PROCESS_H */