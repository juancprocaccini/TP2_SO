#include <stdint.h>
#include "usrlib.h"
#include <stdarg.h>

/* --- Llamadas a ASM --- */

extern void sys_mem_state(MemStats *stats);
extern void *sys_mem_alloc(uint64_t size);
extern void sys_mem_free(void *ptr);
extern int sys_read(char *buf, int n);
extern int sys_write(int fd, const char *buf, int n);
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
extern int sys_pipe_open(int fd, int mode);
extern int sys_pipe_open_free(int mode);
extern int sys_pipe_reserve(void);
extern int sys_pipe_read(int fd, char *buf, int n);
extern int sys_pipe_write(int fd, const char *buf, int n);
extern int sys_pipe_close(int fd);

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

int strlen(const char *s) {
    int n = 0;
    while (*s++) n++;
    return n;
}

int strncasecmp(const char *s1, const char *s2, int n) {
    while (n-- > 0) {
        char a = *s1++, b = *s2++;
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b) return (unsigned char)a - (unsigned char)b;
        if (a == 0) return 0;
    }
    return 0;
}

int satoi(const char *str, int *ok) {
    int val = 0, neg = 0;
    if (ok) *ok = 1;
    if (!str || !*str) { if (ok) *ok = 0; return 0; }
    if (*str == '-') { neg = 1; str++; }
    if (!*str) { if (ok) *ok = 0; return 0; }
    while (*str >= '0' && *str <= '9') val = val * 10 + (*str++ - '0');
    if (*str && ok) *ok = 0;
    return neg ? -val : val;
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


void kprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char buf[32]; // Buffer temporal para conversiones numéricas

    while (*fmt != '\0')
    {
        if (*fmt == '%')
        {
            fmt++; // Avanzamos para ver el especificador
            switch (*fmt)
            {
            case 'd':
            {
                int val = va_arg(args, int);
                intToString(val, buf);
                drawString(buf, 0xFFFFFF); // Color blanco por defecto
                break;
            }
            case 'x':
            case 'X':
            {
                uint64_t val = va_arg(args, uint64_t);
                uint64ToHex(val, buf);
                drawString(buf, 0xFFFFFF);
                break;
            }
            case 's':
            {
                char *str = va_arg(args, char *);
                if (str != 0)
                {
                    drawString(str, 0xFFFFFF);
                }
                else
                {
                    drawString("(null)", 0xFFFFFF);
                }
                break;
            }
            case 'c':
            {
                char ch = (char)va_arg(args, int);
                drawChar(ch, 0xFFFFFF);
                break;
            }
            case '%':
            {
                drawChar('%', 0xFFFFFF);
                break;
            }
            default: // Si es un formato no soportado, imprimimos ambos caracteres
                drawChar('%', 0xFFFFFF);
                drawChar(*fmt, 0xFFFFFF);
                break;
            }
        }
        else if (*fmt == '\n')
        {
            newLine();
        }
        else
        {
            drawChar(*fmt, 0xFFFFFF);
        }
        fmt++;
    }

    va_end(args);
}

/* --- I/O por fd (F7) --- */

int getchar(void) {
    char c;
    if (sys_read(&c, 1) == 0) return 0;    /* EOF (Ctrl+D) */
    return (unsigned char)c;
}

void putchar(char c) {
    sys_write(1, &c, 1);    /* FD_STDOUT = stream index 1 */
}

char *gets(char *buf) {
    int i = 0;
    int c;
    while ((c = getchar()) != '\n' && c != 0) {
        if (c == '\b') {
            if (i > 0) { i--; deleteChar(); }
        } else {
            buf[i++] = (char)c;
            putchar((char)c);
        }
    }
    putchar('\n');   /* avanzar a la siguiente línea (Enter o EOF) */
    buf[i] = '\0';
    return buf;
}

void puts(const char *s) {
    while (*s) putchar(*s++);
    putchar('\n');
}

/* Formatea fmt+args a out[0..max-1], retorna bytes escritos (sin el '\0').
 * Soporta: %[-][N]d  %[-][N]s  %[-][N]x  %c  %% */
static int vfmt(char *out, int max, const char *fmt, va_list args) {
    int pos = 0;
    char tmp[32];

    while (*fmt && pos < max - 1) {
        if (*fmt != '%') {
            out[pos++] = *fmt++;
            continue;
        }
        fmt++;  /* salta '%' */

        /* flag de alineación */
        int left = 0;
        if (*fmt == '-') { left = 1; fmt++; }

        /* ancho mínimo */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');

        const char *s;
        int slen;

        switch (*fmt) {
        case 'd': {
            int val = va_arg(args, int);
            intToString(val, tmp);
            s = tmp; slen = strlen(s);
            break;
        }
        case 'x': case 'X': {
            uint64_t val = va_arg(args, uint64_t);
            uint64ToHex(val, tmp);
            s = tmp; slen = strlen(s);
            break;
        }
        case 's': {
            char *sp = va_arg(args, char *);
            s = sp ? sp : "(null)";
            slen = strlen(s);
            break;
        }
        case 'c': {
            tmp[0] = (char)va_arg(args, int);
            tmp[1] = '\0';
            s = tmp; slen = 1;
            break;
        }
        case '%':
            if (pos < max - 1) out[pos++] = '%';
            fmt++;
            continue;
        default:
            if (pos < max - 1) out[pos++] = '%';
            if (pos < max - 1) out[pos++] = *fmt;
            fmt++;
            continue;
        }

        /* padding a la izquierda (alineación derecha) */
        if (!left) {
            for (int i = slen; i < width && pos < max - 1; i++) out[pos++] = ' ';
        }
        /* valor */
        for (int i = 0; s[i] && pos < max - 1; i++) out[pos++] = s[i];
        /* padding a la derecha (alineación izquierda) */
        if (left) {
            for (int i = slen; i < width && pos < max - 1; i++) out[pos++] = ' ';
        }
        fmt++;
    }
    out[pos] = '\0';
    return pos;
}

int fprintf(int fd, const char *fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int n = vfmt(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n > 0) sys_write(fd, buf, n);
    return n;
}

int printf(const char *fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int n = vfmt(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n > 0) sys_write(1, buf, n);    /* FD_STDOUT */
    return n;
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

/* --- Pipes --- */

int pipe_open(int fd, int mode)
{
    return sys_pipe_open(fd, mode);
}

int pipe_open_free(int mode)
{
    return sys_pipe_open_free(mode);
}

int pipe_reserve(void)
{
    return sys_pipe_reserve();
}

int pipe_read(int fd, char *buf, int n)
{
    return sys_pipe_read(fd, buf, n);
}

int pipe_write(int fd, const char *buf, int n)
{
    return sys_pipe_write(fd, buf, n);
}

int pipe_close(int fd)
{
    return sys_pipe_close(fd);
}