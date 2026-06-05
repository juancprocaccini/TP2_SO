; usrlib.asm — wrappers de syscall para userland
;
; Convención de llamada (int 0x80 via IDT):
;   rax = número de syscall  (ver syscalls.h en Kernel/include)
;   rdi = arg1, rsi = arg2, rdx = arg3, rcx = arg4, r8 = arg5, r9 = arg6
;   Valor de retorno en rax.
;
; TODO (TP2): cuando se implemente separación de privilegios, migrar a la
;   instrucción 'syscall'. En ese caso arg4 deberá ir en r10 (rcx es
;   destruido por el hardware al ejecutar syscall).

; De Video y Tiempo
global drawChar, drawString, putPixel, sys_fillRectangle
global clearScreen, newLine, scrollDown
global moveCursorLeft, moveCursorRight, deleteChar
global increaseFontSize, decreaseFontSize
global kbdGetChar
global getSeconds, getMinutes, getHours
global getDay, getMonth, getYear
global secsToWait, getTime
global vd_drawString, vd_drawIntAt
global getScreenWidth, getScreenHeight

; De Procesos y memoria
global sys_mem_alloc, sys_mem_free, sys_mem_state
global sys_create_process, sys_exit, sys_getpid, sys_yield, sys_waitpid
global sys_kill, sys_nice, sys_block, sys_unblock, sys_ps, sys_free_ps
global sys_get_status, sys_get_my_fds

; De Semáforos
global sys_sem_open, sys_sem_open_get_id, sys_sem_wait, sys_sem_post, sys_sem_close

%macro SYSCALL 1
    mov rax, %1
    int 0x80
    ret
%endmacro

section .text

; --- Video ---
drawChar:
    SYSCALL 1

drawString:
    SYSCALL 2

putPixel:
    SYSCALL 3

sys_fillRectangle:
    SYSCALL 4

clearScreen:
    SYSCALL 7

newLine:
    SYSCALL 8

scrollDown:
    SYSCALL 9

moveCursorLeft:
    SYSCALL 10

moveCursorRight:
    SYSCALL 11

deleteChar:
    SYSCALL 12

increaseFontSize:
    SYSCALL 13

decreaseFontSize:
    SYSCALL 14

vd_drawString:
    SYSCALL 27

vd_drawIntAt:
    SYSCALL 28

getScreenWidth:
    SYSCALL 30

getScreenHeight:
    SYSCALL 31

; --- Teclado ---
kbdGetChar:
    SYSCALL 15

; --- Tiempo / RTC ---
getSeconds:
    SYSCALL 16

getMinutes:
    SYSCALL 17

getHours:
    SYSCALL 18

getDay:
    SYSCALL 19

getMonth:
    SYSCALL 20

getYear:
    SYSCALL 21

secsToWait:
    SYSCALL 22

getTime:
    SYSCALL 23

sys_mem_alloc:
    SYSCALL 32

sys_mem_free:
    SYSCALL 33

sys_mem_state:
    SYSCALL 34

sys_create_process:
    SYSCALL 40

sys_exit:
    SYSCALL 41

sys_getpid:
    SYSCALL 42

sys_yield:
    SYSCALL 43

sys_waitpid:
    SYSCALL 44

sys_kill:
    SYSCALL 45

sys_nice:
    SYSCALL 46

sys_block:
    SYSCALL 47

sys_unblock:
    SYSCALL 48

sys_ps:
    SYSCALL 49

sys_free_ps:
    SYSCALL 50

sys_get_status:
    SYSCALL 51

sys_get_my_fds:
    SYSCALL 52

sys_sem_open:
    SYSCALL 53

sys_sem_open_get_id:
    SYSCALL 54

sys_sem_wait:
    SYSCALL 55

sys_sem_post:
    SYSCALL 56

sys_sem_close:
    SYSCALL 57