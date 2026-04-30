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
