#include "syscalls.h"
#include "defs.h"
#include <stdint.h>
#include <stddef.h>
#include "videoDriver.h"
#include "keyboardDriver.h"
#include "time.h"
#include "mem.h"
#include "mem_user.h"
#include "process.h"
#include "scheduler.h"
#include "semaphore.h"
#include "pipe.h"

static void sys_fillRectangle(uint64_t* args);

uint64_t syscallDispatcher(uint64_t syscall_num, uint64_t arg1, uint64_t arg2,
                           uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    switch (syscall_num) {

        /* --- Video --- */
        case SYS_DRAWCHAR:
            vPutChar(arg1, arg2);
            break;
        case SYS_DRAWSTRING:
            vprintString((const char*)arg1, arg2);
            break;
        case SYS_PUTPIXEL:
            putPixel(arg1, arg2, arg3);
            break;
        case SYS_FILLRECT:
            sys_fillRectangle((uint64_t*)arg1);
            break;
        case SYS_CLEAR_SCREEN:
            clearScreen();
            break;
        case SYS_NEWLINE:
            newLine();
            break;
        case SYS_SCROLLDOWN:
            scrollDown();
            break;
        case SYS_MOVE_LEFT:
            moveCursorLeft();
            break;
        case SYS_MOVE_RIGHT:
            moveCursorRight();
            break;
        case SYS_DELETE_CHAR:
            deleteChar();
            break;
        case SYS_INCREASE_FONT:
            increaseFontSize();
            break;
        case SYS_DECREASE_FONT:
            decreaseFontSize();
            break;
        case SYS_VD_DRAWSTRING:
            vd_drawString(arg1, arg2, (const char*)arg3, arg4, arg5);
            break;
        case SYS_DRAW_INT_AT:
            vd_drawIntAt((int)arg1, (int)arg2, (int)arg3, arg4, arg5, (int)arg6);
            break;
        case SYS_GET_SCREEN_WIDTH:
            return getScreenWidth();
        case SYS_GET_SCREEN_HEIGHT:
            return getScreenHeight();

        /* --- Teclado --- */
        case SYS_KBD_BUFFER_GET:
            return (uint64_t)kbd_buffer_get();

        /* --- Tiempo / RTC --- */
        case SYS_GET_SECONDS:
            return getSeconds();
        case SYS_GET_MINUTES:
            return getMinutes();
        case SYS_GET_HOURS:
            return getHours();
        case SYS_GET_DAY:
            return getDay();
        case SYS_GET_MONTH:
            return getMonth();
        case SYS_GET_YEAR:
            return getYear();
        case SYS_TIMER_WAIT:
            timer_wait(arg1);
            break;
        case SYS_GET_TIME:
            getTime((uint8_t*)arg1);
            break;

        /* --- Memoria --- */
        case SYS_MEM_ALLOC:
            return (uint64_t)user_mem_alloc(arg1); // Rutea a tu User Heap.
        case SYS_MEM_FREE:
            user_mem_free((void *)arg1);
            return 0;
        case SYS_MEM_STATE:
        {
            // arg1 es el puntero al arreglo de 2 MemStats: MemStats info[2]
            MemStats *stats = (MemStats *)arg1;

            if (stats != NULL)
            {
                // stats[0] = Kernel Heap
                mem_state(&stats[0].total, &stats[0].used, &stats[0].free);

                // stats[1] = User Heap
                user_mem_state(&stats[1].total, &stats[1].used, &stats[1].free);
            }
            return 0;
        }
            // Rellena estadísticas de memoria si se requiere
            return 0;

            /* --- Procesos --- */
        case SYS_CREATE_PROCESS:
            return (uint64_t)process_create((entry_t)arg1, (priority_t)arg2, (int)arg3, (char **)arg4, (int)arg5, (int *)arg6);
        case SYS_EXIT:
            process_exit((int)arg1);
            break;
        case SYS_GETPID:
            return (uint64_t)process_getpid();
        case SYS_YIELD:
            process_yield();
            return 0;
        case SYS_WAITPID:
            return (uint64_t)process_waitpid((pid_t)arg1, (int *)arg2);
        case SYS_KILL:
            return (uint64_t)process_kill((pid_t)arg1);
        case SYS_NICE:
            return (uint64_t)process_nice((pid_t)arg1, (priority_t)arg2);
        case SYS_BLOCK:
            return (uint64_t)process_block((pid_t)arg1);
        case SYS_UNBLOCK:
            return (uint64_t)process_unblock((pid_t)arg1);
        case SYS_PS:
            return (uint64_t)process_ps((ProcessInfoList **)arg1);
        case SYS_FREE_PS:
            process_free_ps((ProcessInfoList *)arg1);
            return 0;
        case SYS_GET_STATUS:
            return (uint64_t)process_get_status((pid_t)arg1);
        case SYS_GET_MY_FDS:
            process_get_my_fds((int *)arg1);
            return 0;

            /* --- Semáforos --- */
        case SYS_SEM_OPEN:
            return (uint64_t)ksem_open((int)arg1, arg2);

        case SYS_SEM_OPEN_GET_ID:
            return (uint64_t)ksem_open_get_id(arg1);

        case SYS_SEM_WAIT:
            return (uint64_t)ksem_wait((int)arg1);

        case SYS_SEM_POST:
            return (uint64_t)ksem_post((int)arg1);

        case SYS_SEM_CLOSE:
            return (uint64_t)ksem_close((int)arg1);

            /* --- Pipes --- */

        case SYS_PIPE_OPEN:
            // Traduce el fd de entrada (id + 3) a ID interno del arreglo de pipes
            return (uint64_t)pipe_open((int)arg1 - 3, (int)arg2);

        case SYS_PIPE_OPEN_FREE:
        {
            int pipe_id = pipe_open_free((int)arg1);
            // Retorna el fd mapeado para Userland (id + 3) o -1 si falló
            return (pipe_id >= 0) ? (uint64_t)(pipe_id + 3) : -1;
        }

        case SYS_PIPE_RESERVE:
        {
            int pipe_id = pipe_reserve();
            return (pipe_id >= 0) ? (uint64_t)(pipe_id + 3) : -1;
        }

        case SYS_PIPE_READ:
            return (uint64_t)pipe_read((int)arg1 - 3, (char *)arg2, (int)arg3);

        case SYS_PIPE_WRITE:
            return (uint64_t)pipe_write((int)arg1 - 3, (const char *)arg2, (int)arg3);

        case SYS_PIPE_CLOSE:
            return (uint64_t)pipe_close((int)arg1 - 3, scheduler_get_running()->pid);

        /* --- I/O por fd (F7) --- */
        case SYS_READ:
        {
            /* arg1=buf, arg2=n — lee fds[STDIN] del proceso corriendo */
            PCB *running = scheduler_get_running();
            int fd = running->fds[FD_STDIN];
            if (fd < 0) return 0;                                   /* stdin cerrado → EOF */
            if (fd <= 2) return (uint64_t)stdin_read((char *)arg1, (int)arg2);
            return (uint64_t)pipe_read(fd - 3, (char *)arg1, (int)arg2);
        }

        case SYS_WRITE:
        {
            /* arg1=stream_idx(0/1/2), arg2=buf, arg3=n */
            int idx = (int)arg1;
            if (idx < 0 || idx > 2) return (uint64_t)-1;
            PCB *running = scheduler_get_running();
            int fd = running->fds[idx];
            char *buf = (char *)arg2;
            int n = (int)arg3;
            if (fd < 0) return (uint64_t)n;                         /* fd cerrado → descartar */
            if (fd <= 2) {
                for (int i = 0; i < n; i++) {
                    char c = buf[i];
                    if (c == '\n') newLine();
                    else if (c == '\b') deleteChar();
                    else vPutChar((uint64_t)(unsigned char)c, 0xFFFFFF);
                }
                return (uint64_t)n;
            }
            return (uint64_t)pipe_write(fd - 3, buf, n);
        }

        case SYS_REAP_ORPHANS:
            process_reap_orphans();
            return 0;

        default:
            return -1;
    }
    return 0;
}

static void sys_fillRectangle(uint64_t* args) {
    fillRectangle(args[0], args[1], args[2], args[3], args[4]);
}
