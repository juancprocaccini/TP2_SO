#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "defs.h"

typedef int pid_t;

typedef enum { LOW = 0, MEDIUM, HIGH } priority_t;
typedef enum { FREE = 0, READY, BLOCKED, ZOMBIE } pstate_t;

typedef struct PCB {
    pid_t      pid;
    pstate_t   state;
    priority_t priority;
    uint64_t   rsp;
    uint64_t   stack_base;
    int        argc;
    char     **argv;
    char       name[32];
    
    /* 0=stdin, 1=stdout, 2=stderr. Valores >= 3 indican (pipe_id + 3) */
    int        fds[3];
    
    int64_t    exit_status;
    int        killable;           /* idle = 0 */
    struct PCB *waiting_me;        /* Proceso que me hizo wait() */
    struct PCB *waiting_for;       /* Proceso por el que estoy haciendo wait() */
    
    /* -1 si no está esperando un semáforo. Otro valor indica el ID del semáforo */
    int64_t    blocked_by_sem;
} PCB;

typedef int (*entry_t)(char **argv, int argc);

pid_t process_create(entry_t rip, priority_t pri, int killable, char **argv, int argc, int fds[3]);
void  process_wrapper(entry_t rip, char **argv, int argc, pid_t pid);

void  process_exit(int status);
pid_t process_getpid(void);
void  process_yield(void);

int   process_kill(pid_t pid);
int   process_nice(pid_t pid, priority_t priority);
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