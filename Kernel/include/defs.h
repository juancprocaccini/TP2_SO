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
#define MAX_SEMAPHORES  64
#define MAX_PIPES       32
#define MAX_PRIO        5
#define MAX_WAITERS     MAX_PROCESSES

#define STACK_SIZE      16384

/*
 * Layout de memoria:
 *   0x100000 .. ~0x300000   kernel (.text, .rodata, .data, .bss)
 *   0x400000 .. <0x600000   imagen del módulo de userland
 *   0x600000 .. 0x2000000   heap del kernel (28 MB) — gestionado por mem_alloc
 */
#define HEAP_START      0x600000UL
#define HEAP_END        0x2000000UL
#define HEAP_SIZE       (HEAP_END - HEAP_START)   /* 28 MB */

/* File descriptors estándar */
#define FD_STDIN        0
#define FD_STDOUT       1
#define FD_STDERR       2

/* Identificadores internos del kernel para FDs reservados */
#define KFD_STDIN       (-2)
#define KFD_STDOUT      (-3)
#define KFD_STDERR      (-4)

#endif
