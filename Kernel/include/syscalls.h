#ifndef SYSCALLS_H
#define SYSCALLS_H
#include <stdint.h>

/*
 * Syscall numbers — definidos acá y usados tanto en kernel (syscallDispatcher.c)
 * como en userland (usrlib.asm, que debe tener los mismos números).
 *
 * Convención de llamada (int 0x80 via IDT):
 *   rax = número de syscall
 *   rdi = arg1, rsi = arg2, rdx = arg3, rcx = arg4, r8 = arg5, r9 = arg6
 *
 * TODO (TP2): migrar a instrucción 'syscall' con LSTAR/STAR MSRs al implementar
 * procesos con separación de privilegios. En ese caso arg4 pasa por r10
 * (rcx es destruido por la instrucción syscall del hardware).
 */

typedef struct
{
    uint64_t total;
    uint64_t used;
    uint64_t free;
} MemStats;

/* --- Video --- */
#define SYS_DRAWCHAR        1
#define SYS_DRAWSTRING      2
#define SYS_PUTPIXEL        3
#define SYS_FILLRECT        4
/* 5, 6 reservados */
#define SYS_CLEAR_SCREEN    7
#define SYS_NEWLINE         8
#define SYS_SCROLLDOWN      9
#define SYS_MOVE_LEFT       10
#define SYS_MOVE_RIGHT      11
#define SYS_DELETE_CHAR     12
#define SYS_INCREASE_FONT   13
#define SYS_DECREASE_FONT   14

/* --- Teclado --- */
#define SYS_KBD_BUFFER_GET  15

/* --- Tiempo / RTC --- */
#define SYS_GET_SECONDS     16
#define SYS_GET_MINUTES     17
#define SYS_GET_HOURS       18
#define SYS_GET_DAY         19
#define SYS_GET_MONTH       20
#define SYS_GET_YEAR        21
#define SYS_TIMER_WAIT      22
#define SYS_GET_TIME        23

/* --- Video con coordenadas --- */
#define SYS_VD_DRAWSTRING   27
#define SYS_DRAW_INT_AT     28

/* --- Resolución de pantalla --- */
#define SYS_GET_SCREEN_WIDTH  30
#define SYS_GET_SCREEN_HEIGHT 31

/* --- Memoria --- */
#define SYS_MEM_ALLOC 32
#define SYS_MEM_FREE  33
#define SYS_MEM_STATE 34

/* --- Procesos --- */
#define SYS_CREATE_PROCESS 40
#define SYS_EXIT           41
#define SYS_GETPID         42
#define SYS_YIELD          43
#define SYS_WAITPID        44
#define SYS_KILL           45
#define SYS_NICE           46
#define SYS_BLOCK          47
#define SYS_UNBLOCK        48
#define SYS_PS             49
#define SYS_FREE_PS        50
#define SYS_GET_STATUS     51
#define SYS_GET_MY_FDS     52

/* --- Semáforos --- */
#define SYS_SEM_OPEN        53
#define SYS_SEM_OPEN_GET_ID 54 
#define SYS_SEM_WAIT        55 
#define SYS_SEM_POST        56 
#define SYS_SEM_CLOSE       57

/* ----------------------------------------------------------------
 * TODO (TP2): agregar los siguientes grupos de syscalls
 * ---------------------------------------------------------------- */
/* --- IPC (pipes) --- */
#define SYS_PIPE_OPEN       58
#define SYS_PIPE_OPEN_FREE  59
#define SYS_PIPE_RESERVE    60
#define SYS_PIPE_READ       61
#define SYS_PIPE_WRITE      62
#define SYS_PIPE_CLOSE      63

#endif
