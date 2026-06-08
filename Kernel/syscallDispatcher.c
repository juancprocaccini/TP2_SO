/*
 * syscallDispatcher.c — despacha syscalls recibidas vía int 0x80.
 *
 * Convención de llamada (ver usrlib.asm y syscalls.h):
 *   rax = número de syscall
 *   rdi/rsi/rdx/rcx/r8/r9 = arg1..arg6
 *   Valor de retorno en rax.
 *
 * Regla: casos void usan break; casos con valor de retorno usan return.
 * El default retorna -1 para syscalls no implementadas.
 */

#include "syscalls.h"
#include <stdint.h>
#include "videoDriver.h"
#include "keyboardDriver.h"
#include "time.h"
#include <mem.h>
#include <mem_user.h>
#include <process.h>
#include <scheduler.h>
#include <stddef.h>

static void sys_fillRectangle(uint64_t *args);

uint64_t syscallDispatcher(uint64_t syscall_num, uint64_t arg1, uint64_t arg2,
                           uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    switch (syscall_num)
    {

        /* ----------------------------------------------------------------
         * Video
         * ---------------------------------------------------------------- */

    case SYS_DRAWCHAR:
        vPutChar(arg1, arg2);
        break;

    case SYS_DRAWSTRING:
        vprintString((const char *)arg1, arg2);
        break;

    case SYS_PUTPIXEL:
        putPixel(arg1, arg2, arg3);
        break;

    case SYS_FILLRECT:
        sys_fillRectangle((uint64_t *)arg1);
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
        vd_drawString(arg1, arg2, (const char *)arg3, arg4, arg5);
        break;

    case SYS_DRAW_INT_AT:
        vd_drawIntAt((int)arg1, (int)arg2, (int)arg3, arg4, arg5, (int)arg6);
        break;

    case SYS_GET_SCREEN_WIDTH:
        return getScreenWidth();

    case SYS_GET_SCREEN_HEIGHT:
        return getScreenHeight();

        /* ----------------------------------------------------------------
         * Teclado
         * ---------------------------------------------------------------- */

    case SYS_KBD_BUFFER_GET:
        return (uint64_t)kbd_buffer_get();

        /* ----------------------------------------------------------------
         * Tiempo / RTC
         * ---------------------------------------------------------------- */

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
        getTime((uint8_t *)arg1);
        break;

        /* ----------------------------------------------------------------
         * Memoria (F4)
         * ---------------------------------------------------------------- */

    case SYS_MEM_ALLOC:
        /* arg1 = size; retorna puntero al user heap o NULL */
        return (uint64_t)user_mem_alloc((uint64_t)arg1);

    case SYS_MEM_FREE:
        /* arg1 = ptr */
        user_mem_free((void *)arg1);
        break;

    case SYS_MEM_STATE:
    {
        /*
         * arg1 = puntero a uint64_t[6].
         * Layout: [u_total, u_used, u_free, k_total, k_used, k_free]
         * Userland usa estos seis valores para el comando 'mem'.
         */
        uint64_t *out = (uint64_t *)arg1;
        user_mem_state(&out[0], &out[1], &out[2]);
        mem_state(&out[3], &out[4], &out[5]);
        break;
    }

        /* ----------------------------------------------------------------
         * Procesos (F4)
         * ---------------------------------------------------------------- */

    case SYS_CREATE_PROCESS:
    {
        /*
         * arg1 = entry point, arg2 = priority, arg3 = argv,
         * arg4 = argc,        arg5 = fds[3]
         *
         * process_create copia argv internamente; el caller no necesita
         * mantener el buffer vivo después de esta syscall.
         */
        entry_t entry = (entry_t)arg1;
        priority_t pri = (priority_t)arg2;
        char **argv = (char **)arg3;
        int argc = (int)arg4;
        int *fds = (int *)arg5;
        return (uint64_t)process_create(entry, pri, 1, argv, argc, fds);
    }

    case SYS_EXIT:
        /*
         * process_exit desplanifica el running, notifica a su waiter,
         * fuerza context switch y no retorna.
         */
        process_exit((int)arg1);
        while (1)
            ; /* silencia el warning de función no-void */

    case SYS_GETPID:
        return (uint64_t)process_getpid();

    case SYS_YIELD:
        process_yield();
        break;

    case SYS_WAITPID:
    {
        /*
         * arg1 = pid objetivo, arg2 = puntero int* donde escribir
         * el exit status (puede ser NULL).
         */
        pid_t target = (pid_t)arg1;
        int *ret_out = (int *)arg2;
        return (uint64_t)process_waitpid(target, ret_out);
    }

    case SYS_KILL:
        return (uint64_t)process_kill((pid_t)arg1);

    case SYS_NICE:
        return (uint64_t)process_nice((pid_t)arg1, (priority_t)arg2);

    case SYS_BLOCK:
        return (uint64_t)process_block((pid_t)arg1);

    case SYS_UNBLOCK:
        return (uint64_t)process_unblock((pid_t)arg1);

    case SYS_GET_STATUS:
        return (uint64_t)process_get_status((pid_t)arg1);

    case SYS_PS:
    {
        /*
         * arg1 = ProcessInfoList** donde guardar el puntero.
         * La lista es alocada en el kernel heap; userland la libera
         * con SYS_FREE_PS para que mem_free corra en contexto kernel.
         */
        ProcessInfoList **out = (ProcessInfoList **)arg1;
        *out = process_ps();
        return (*out != NULL) ? 0 : (uint64_t)-1;
    }

    case SYS_FREE_PS:
        process_free_ps((ProcessInfoList *)arg1);
        break;

    case SYS_GET_MY_FDS:
    {
        /*
         * arg1 = int[3]* donde copiar los fds del proceso running.
         * Necesario para que userland conozca sus propios descriptores
         * sin acceder directamente al PCB.
         */
        int *out = (int *)arg1;
        PCB *p = process_get(process_getpid());
        out[0] = p->fds[0];
        out[1] = p->fds[1];
        out[2] = p->fds[2];
        break;
    }

        /* ----------------------------------------------------------------
         * Semáforos — reservados F5
         * ---------------------------------------------------------------- */
        /* case SYS_SEM_OPEN: ... */
        /* case SYS_SEM_WAIT: ... */
        /* case SYS_SEM_POST: ... */
        /* case SYS_SEM_CLOSE: ... */

        /* ----------------------------------------------------------------
         * Pipes — reservados F6
         * ---------------------------------------------------------------- */
        /* case SYS_PIPE_OPEN: ... */
        /* case SYS_PIPE_READ: ... */
        /* case SYS_PIPE_WRITE: ... */
        /* case SYS_PIPE_CLOSE: ... */

        /* ----------------------------------------------------------------
         * FD abstraction — reservado F7
         * ---------------------------------------------------------------- */
        /* case SYS_READ: ... */
        /* case SYS_WRITE: ... */

    default:
        return (uint64_t)-1;
    }

    return 0;
}

static void sys_fillRectangle(uint64_t *args)
{
    fillRectangle(args[0], args[1], args[2], args[3], args[4]);
}