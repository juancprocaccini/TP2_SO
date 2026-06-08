; usrlib.asm — wrappers de syscall para userland
;
; Convención de llamada (int 0x80 via IDT):
;   rax = número de syscall  (ver syscalls.h en Kernel/include)
;   rdi = arg1, rsi = arg2, rdx = arg3, rcx = arg4, r8 = arg5, r9 = arg6
;   Valor de retorno en rax.

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

; --- F4: Memoria ---
global sys_malloc, sys_free, sys_mem_state

; --- F4: Procesos ---
global sys_create_process, sys_exit, sys_getpid, sys_yield
global sys_waitpid, sys_kill, sys_nice
global sys_block, sys_unblock, sys_get_status
global sys_ps, sys_free_ps, sys_get_my_fds

%macro SYSCALL 1
    mov rax, %1
    int 0x80
    ret
%endmacro

section .text

; ----------------------------------------------------------------
; Video
; ----------------------------------------------------------------
drawChar:           SYSCALL 1
drawString:         SYSCALL 2
putPixel:           SYSCALL 3
sys_fillRectangle:  SYSCALL 4
clearScreen:        SYSCALL 7
newLine:            SYSCALL 8
scrollDown:         SYSCALL 9
moveCursorLeft:     SYSCALL 10
moveCursorRight:    SYSCALL 11
deleteChar:         SYSCALL 12
increaseFontSize:   SYSCALL 13
decreaseFontSize:   SYSCALL 14
vd_drawString:      SYSCALL 27
vd_drawIntAt:       SYSCALL 28
getScreenWidth:     SYSCALL 30
getScreenHeight:    SYSCALL 31

; ----------------------------------------------------------------
; Teclado
; ----------------------------------------------------------------
kbdGetChar:         SYSCALL 15

; ----------------------------------------------------------------
; Tiempo / RTC
; ----------------------------------------------------------------
getSeconds:         SYSCALL 16
getMinutes:         SYSCALL 17
getHours:           SYSCALL 18
getDay:             SYSCALL 19
getMonth:           SYSCALL 20
getYear:            SYSCALL 21
secsToWait:         SYSCALL 22
getTime:            SYSCALL 23

; ----------------------------------------------------------------
; Memoria (F4) — syscalls 32-34
; ----------------------------------------------------------------

sys_malloc:         ; rdi=size → rax=ptr (user heap)
    SYSCALL 32

sys_free:           ; rdi=ptr
    SYSCALL 33

sys_mem_state:      ; rdi=uint64_t[6]*  → [u_tot,u_used,u_free, k_tot,k_used,k_free]
    SYSCALL 34

; ----------------------------------------------------------------
; Procesos (F4) — syscalls 40-52
; ----------------------------------------------------------------

sys_create_process:
    ; rdi=entry, rsi=priority, rdx=argv, rcx=argc, r8=fds[3]*
    ; El dispatcher recibe arg4 en rcx — convención int 0x80 estándar
    SYSCALL 40

sys_exit:
    ; rdi=status  — no retorna
    SYSCALL 41

sys_getpid:
    ; → rax=pid
    SYSCALL 42

sys_yield:
    ; sin args, sin retorno útil
    SYSCALL 43

sys_waitpid:
    ; rdi=pid, rsi=int* ret_out  → rax=pid_reapeado / -1
    SYSCALL 44

sys_kill:
    ; rdi=pid  → rax=0/-1
    SYSCALL 45

sys_nice:
    ; rdi=pid, rsi=new_priority  → rax=0/-1
    SYSCALL 46

sys_block:
    ; rdi=pid  → rax=0/-1
    SYSCALL 47

sys_unblock:
    ; rdi=pid  → rax=0/-1
    SYSCALL 48

sys_get_status:
    ; rdi=pid  → rax=pstate_t / -1
    SYSCALL 51

sys_ps:
    ; rdi=ProcessInfoList**  → rax=0/-1
    SYSCALL 49

sys_free_ps:
    ; rdi=ProcessInfoList*
    SYSCALL 50

sys_get_my_fds:
    ; rdi=int[3]*
    SYSCALL 52