#ifndef USRLIB_H
#define USRLIB_H

#include <stdint.h>

/* ----------------------------------------------------------------
 * Video
 * ---------------------------------------------------------------- */
extern void drawChar(uint64_t ch, uint64_t color);
extern void drawString(const char* s, uint64_t color);
extern void putPixel(uint64_t hexColor, uint64_t x, uint64_t y);
extern void sys_fillRectangle(uint64_t *info);
void fillRectangle(int x, int y, int width, int height, uint32_t color);
extern void clearScreen(void);
extern void newLine(void);
extern void scrollDown(void);
extern void moveCursorLeft(void);
extern void moveCursorRight(void);
extern void deleteChar(void);
extern void increaseFontSize(void);
extern void decreaseFontSize(void);
extern void vd_drawString(uint64_t x, uint64_t y, const char *s,
                          uint64_t color, uint64_t size);
extern void vd_drawIntAt(int x, int y, int val, uint64_t color,
                         uint64_t size, int right_align);
extern uint32_t getScreenWidth(void);
extern uint32_t getScreenHeight(void);

/* ----------------------------------------------------------------
 * Teclado
 * ---------------------------------------------------------------- */
extern char kbdGetChar(void);

/* ----------------------------------------------------------------
 * Tiempo / RTC
 * ---------------------------------------------------------------- */
extern uint64_t getSeconds(void);
extern uint64_t getMinutes(void);
extern uint64_t getHours(void);
extern uint64_t getDay(void);
extern uint64_t getMonth(void);
extern uint64_t getYear(void);
extern void getTime(uint8_t *buffer);
extern void secsToWait(int seconds);

/* ----------------------------------------------------------------
 * Memoria (F4)
 * ---------------------------------------------------------------- */

/* sys_malloc/sys_free operan sobre el user heap vía SYS_MEM_ALLOC/FREE. */
extern void *sys_malloc(uint64_t size);
extern void sys_free(void *ptr);

/*
 * sys_mem_state llena out[6]:
 *   out[0..2] = user heap  { total, used, free }
 *   out[3..5] = kernel heap { total, used, free }
 */
extern void sys_mem_state(uint64_t *out);

/* ----------------------------------------------------------------
 * Procesos (F4)
 *
 * Los tipos ProcessInfo / ProcessInfoList replican exactamente los
 * definidos en Kernel/include/process.h — deben mantenerse en sincronía.
 * No incluimos process.h desde userland: es un header de kernel.
 * ---------------------------------------------------------------- */

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

typedef struct
{
    int pid;
    char name[32];
    int priority;
    int state; /* pstate_t casteado a int */
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
 * Crea un proceso hijo. entry es la función que correrá como proceso.
 * fds[3] = { stdin_fd, stdout_fd, stderr_fd }; pasar NULL para heredad
 * los del padre (0/1/2).
 * Retorna pid del hijo, o -1 en error.
 */
extern int sys_create_process(void *entry, int priority,
                              char **argv, int argc, int *fds);

/* Termina el proceso actual con el código dado. No retorna. */
extern void sys_exit(int status);

/* Retorna el PID del proceso actual. */
extern int sys_getpid(void);

/* Cede voluntariamente el CPU. */
extern void sys_yield(void);

/*
 * Espera a que pid termine y escribe su exit status en *ret_out (puede
 * ser NULL). Retorna pid en éxito, -1 en error.
 */
extern int sys_waitpid(int pid, int *ret_out);

/* Mata pid. Retorna 0 / -1. */
extern int sys_kill(int pid);

/* Cambia la prioridad de pid. new_priority: LOW=0, MEDIUM=1, HIGH=2. */
extern int sys_nice(int pid, int new_priority);

/* Bloquea / desbloquea pid. Retorna 0 / -1. */
extern int sys_block(int pid);
extern int sys_unblock(int pid);

/* Retorna el estado de pid (pstate_t como int), o -1 si inválido. */
extern int sys_get_status(int pid);

/*
 * Llena *out con un puntero a la lista de procesos activos (alocada en
 * el kernel heap). Liberar con sys_free_ps() cuando ya no se necesite.
 * Retorna 0 / -1.
 */
extern int sys_ps(ProcessInfoList **out);
extern void sys_free_ps(ProcessInfoList *list);

/* Copia los fds del proceso actual en out[3]. */
extern void sys_get_my_fds(int *out);

/* ----------------------------------------------------------------
 * Utilidades (usrlib.c)
 * ---------------------------------------------------------------- */
int strcmp(const char *str1, const char *str2);
void shell_print(char *str, uint32_t color);
void getTimeString(char *buffer);
void getDateString(char *buffer);
void intToString(int value, char *buffer);
void uint64ToHex(uint64_t value, char *buffer);

/*
 * printf mínimo: soporta %d %u %x %s %c %%.
 * Escribe en stdout (color blanco fijo). Para colorear usar drawString.
 */
void printf(const char *fmt, ...);

#endif /* USRLIB_H */