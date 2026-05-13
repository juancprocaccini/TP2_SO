#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>

/*
 * Constantes globales del kernel — TP2.
 * Cualquier subsistema (mem, processes, sync, ipc) que necesite tamaños fijos
 * debe importar este header en lugar de redefinir.
 */

#define MAX_PROCESSES   32
#define MAX_FDS         16
#define MAX_PIPES       64
#define MAX_PRIO        3
#define MAX_WAITERS     MAX_PROCESSES

#define STACK_SIZE      16384

/* Semaphore namespace */
#define SEM_USER_MAX    100   /* user-accessible ids: 0..99 */
#define SEM_KERNEL_MAX  350   /* kernel-only ids:   100..349 (pipe internals) */

/*
 * Layout de memoria:
 *   0x100000 .. ~0x300000   kernel (.text, .rodata, .data, .bss)
 *   0x400000 .. <0x600000   imagen del módulo de userland
 *   0x600000 .. 0x1600000   user heap  (16 MB)
 *   0x1600000.. 0x2600000   kernel heap (16 MB)
 */
#define USER_HEAP_START   0x600000UL
#define USER_HEAP_SIZE    0x1000000UL   /* 16 MB */
#define KERNEL_HEAP_START (USER_HEAP_START + USER_HEAP_SIZE)   /* 0x1600000 */
#define KERNEL_HEAP_SIZE  0x1000000UL   /* 16 MB */

/* File descriptors estándar */
#define FD_STDIN        0
#define FD_STDOUT       1
#define FD_STDERR       2

/* Identificadores internos del kernel para FDs reservados */
#define KFD_STDIN       (-2)
#define KFD_STDOUT      (-3)
#define KFD_STDERR      (-4)

#endif
