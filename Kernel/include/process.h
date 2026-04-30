#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

/*
 * Estructuras y funciones de procesos.
 * Implementaciones en Kernel/processes/.
 */

// typedef enum { RUNNING, READY, BLOCKED, ZOMBIE } ProcessState;

// typedef struct {
//     uint64_t pid;
//     uint64_t ppid;
//     ProcessState state;
//     int priority;
//     uint64_t rsp;
//     uint64_t rbp;
//     uint64_t stack_base;
//     char name[32];
// } PCB;

// int  process_create(void (*entry)(int, char**), int argc, char** argv,
//                     int priority, int is_foreground, int fds[2]);
// void process_exit(int status);
// int  process_getpid(void);
// void process_yield(void);
// void process_block(uint64_t pid);
// void process_unblock(uint64_t pid);
// void process_kill(uint64_t pid);
// void process_nice(uint64_t pid, int priority);
// void process_print_all(void);

#endif
