#include <stdint.h>
#include "usrlib.h"

/* --- Llamadas a ASM --- */

extern void sys_mem_state(MemStats *stats);
extern void *sys_mem_alloc(uint64_t size);
extern void sys_mem_free(void *ptr);
extern int sys_create_process(uint64_t entry, int pri, int killable, char **argv, int argc, int *fds);
extern void sys_exit(int status);
extern int sys_getpid(void);
extern void sys_yield(void);
extern int sys_waitpid(int pid, int *ret_out);
extern int sys_kill(int pid);
extern int sys_nice(int pid, int new_pri);
extern int sys_block(int pid);
extern int sys_unblock(int pid);
extern int sys_ps(ProcessInfoList **info_list_out);
extern void sys_free_ps(ProcessInfoList *list);
extern int sys_get_status(int pid);
extern void sys_get_my_fds(int fds_out[3]);
extern int sys_sem_open(int id, uint64_t initial);
extern int sys_sem_open_get_id(uint64_t initial);
extern int sys_sem_wait(int id);
extern int sys_sem_post(int id);
extern int sys_sem_close(int id);

/* --- Utilidad General --- */

void fillRectangle(int x, int y, int width, int height, uint32_t color) {
    uint64_t info[] = {x, y, width, height, color};
    sys_fillRectangle(info);
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (unsigned char)(*str1) - (unsigned char)(*str2);
}

void shell_print(char* str, uint32_t color) {
    drawString(str, color);
    newLine();
}

void getTimeString(char* buffer) {
    uint64_t h = getHours();
    uint64_t m = getMinutes();
    uint64_t s = getSeconds();
    buffer[0] = (h / 10) + '0';
    buffer[1] = (h % 10) + '0';
    buffer[2] = ':';
    buffer[3] = (m / 10) + '0';
    buffer[4] = (m % 10) + '0';
    buffer[5] = ':';
    buffer[6] = (s / 10) + '0';
    buffer[7] = (s % 10) + '0';
    buffer[8] = '\0';
}

void getDateString(char* buffer) {
    uint64_t d = getDay();
    uint64_t m = getMonth();
    uint64_t y = getYear();
    buffer[0] = (d / 10) + '0';
    buffer[1] = (d % 10) + '0';
    buffer[2] = '/';
    buffer[3] = (m / 10) + '0';
    buffer[4] = (m % 10) + '0';
    buffer[5] = '/';
    buffer[6] = ((y / 1000) % 10) + '0';
    buffer[7] = ((y / 100)  % 10) + '0';
    buffer[8] = ((y / 10)   % 10) + '0';
    buffer[9] = (y % 10) + '0';
    buffer[10] = '\0';
}

void uint64ToHex(uint64_t value, char* buffer) {
    buffer[0] = '0';
    buffer[1] = 'x';
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        buffer[17 - i] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
    }
    buffer[18] = '\0';
}

void intToString(int value, char* buffer) {
    if (value == 0) { buffer[0] = '0'; buffer[1] = '\0'; return; }
    int neg = (value < 0);
    if (neg) value = -value;
    char tmp[12];
    int i = 0;
    while (value > 0) { tmp[i++] = '0' + (value % 10); value /= 10; }
    int j = 0;
    if (neg) buffer[j++] = '-';
    while (i > 0) buffer[j++] = tmp[--i];
    buffer[j] = '\0';
}

void cmd_mem()
{
    MemStats stats[2];
    sys_mem_state(stats); // Llamada a la syscall en ASM

    shell_print("=== KERNEL HEAP ===", 0xFFFFFF);
    // ... imprimís stats[0].total, stats[0].used, etc ...

    shell_print("=== USER HEAP ===", 0xFFFFFF);
    // ... imprimís stats[1].total, stats[1].used, etc ...
}

/* --- Procesos --- */

void *malloc(uint64_t size) { return sys_mem_alloc(size); }

void free(void *ptr) { sys_mem_free(ptr); }

int create_process(int (*entry)(char **, int), int pri, int killable, char **argv, int argc, int fds[3])
{
    return sys_create_process((uint64_t)entry, pri, killable, argv, argc, fds);
}

void exit(int status) { sys_exit(status); }

int getpid(void) { return sys_getpid(); }

void yield(void) { sys_yield(); }

int waitpid(int pid, int *ret_out) { return sys_waitpid(pid, ret_out); }

int kill(int pid) { return sys_kill(pid); }

int nice(int pid, int new_pri) { return sys_nice(pid, new_pri); }

int block(int pid) { return sys_block(pid); }

int unblock(int pid) { return sys_unblock(pid); }

int ps(ProcessInfoList **info_list_out) { return sys_ps(info_list_out); }

void free_ps(ProcessInfoList *list) { sys_free_ps(list); }

int get_status(int pid) { return sys_get_status(pid); }

void get_my_fds(int fds_out[3]) { sys_get_my_fds(fds_out); }

/* --- Semáforos --- */

int sem_open(int id, uint64_t initial)
{
    return sys_sem_open(id, initial);
}

int sem_open_get_id(uint64_t initial)
{
    return sys_sem_open_get_id(initial);
}

int sem_wait(int id)
{
    return sys_sem_wait(id);
}

int sem_post(int id)
{
    return sys_sem_post(id);
}

int sem_close(int id)
{
    return sys_sem_close(id);
}