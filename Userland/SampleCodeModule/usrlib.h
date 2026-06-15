#ifndef USRLIB_H
#define USRLIB_H

#define READER 0
#define WRITER 1

#include <stdint.h>



/* --- Video --- */
extern void drawChar(uint64_t ch, uint64_t color);
extern void drawString(const char* s, uint64_t color);
extern void putPixel(uint64_t hexColor, uint64_t x, uint64_t y);
extern void sys_fillRectangle(uint64_t* info);
void fillRectangle(int x, int y, int width, int height, uint32_t color);
extern void clearScreen(void);
extern void newLine(void);
extern void scrollDown(void);
extern void moveCursorLeft(void);
extern void moveCursorRight(void);
extern void deleteChar(void);
extern void increaseFontSize(void);
extern void decreaseFontSize(void);
extern void vd_drawString(uint64_t x, uint64_t y, const char* s, uint64_t color, uint64_t size);
extern void vd_drawIntAt(int x, int y, int val, uint64_t color, uint64_t size, int right_align);
extern uint32_t getScreenWidth(void);
extern uint32_t getScreenHeight(void);


/* --- Teclado --- */
extern char kbdGetChar(void);

/* --- Tiempo / RTC --- */
extern uint64_t getSeconds(void);
extern uint64_t getMinutes(void);
extern uint64_t getHours(void);
extern uint64_t getDay(void);
extern uint64_t getMonth(void);
extern uint64_t getYear(void);
extern void getTime(uint8_t* buffer);
extern void secsToWait(int seconds);

/* --- Utilidades (usrlib.c) --- */
int strcmp(const char* str1, const char* str2);
int strlen(const char *s);
int strncasecmp(const char *s1, const char *s2, int n);
void shell_print(char* str, uint32_t color);
void getTimeString(char* buffer);
void getDateString(char* buffer);
void intToString(int value, char* buffer);
void uint64ToHex(uint64_t value, char* buffer);
void kprintf(const char *fmt, ...);
int satoi(const char *str, int *ok);

/* --- I/O por fd (F7) --- */
int getchar(void);
void putchar(char c);
char *gets(char *buf, int size);
void puts(const char *s);
int printf(const char *fmt, ...);
int fprintf(int fd, const char *fmt, ...);

/* --- Memoria --- */
typedef struct
{
    uint64_t total;
    uint64_t used;
    uint64_t free;
} MemStats;

extern void sys_mem_state(MemStats *stats);

/* --- Procesos --- */

typedef enum
{
    U_LOW = 0,
    U_MEDIUM,
    U_HIGH
} u_priority_t;

typedef enum
{
    U_FREE = 0,
    U_READY,
    U_BLOCKED,
    U_ZOMBIE
} u_pstate_t;

typedef struct ProcessInfo
{
    int pid;
    char name[32];
    int priority;
    int state;
    uint64_t rsp;
    uint64_t stack_base;
    int foreground;
    int fds[3];
} ProcessInfo;

typedef struct ProcessInfoList
{
    int count;
    ProcessInfo *entries;
} ProcessInfoList;

void *malloc(uint64_t size);
void free(void *ptr);

int create_process(int (*entry)(char **, int), int pri, int killable, char **argv, int argc, int fds[3]);
void exit(int status);
int getpid(void);
void yield(void);
int waitpid(int pid, int *ret_out);
int kill(int pid);
int nice(int pid, int new_pri);
int block(int pid);
int unblock(int pid);
int ps(ProcessInfoList **info_list_out);
void free_ps(ProcessInfoList *list);
int get_status(int pid);
void get_my_fds(int fds_out[3]);

/* --- Semáforos --- */
int sem_open(int id, uint64_t initial);
int sem_open_get_id(uint64_t initial);
int sem_wait(int id);
int sem_post(int id);
int sem_close(int id);

/* --- Pipes --- */
int pipe_open(int fd, int mode);
int pipe_open_free(int mode);
int pipe_reserve(void);
int pipe_read(int fd, char *buf, int n);
int pipe_write(int fd, const char *buf, int n);
int pipe_close(int fd);

/* --- Gestión de zombies --- */
void reap(void);
#endif

