GLOBAL _exception0Handler
GLOBAL _exception6Handler

EXTERN exceptionDispatcher
EXTERN reg_snapshot
EXTERN snapshot_available

%macro pushState 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popState 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

; Exception 0 — Division By Zero
_exception0Handler:
    pushState
    mov rdi, 0
    mov rsi, rsp
    call exceptionDispatcher
    popState
    iretq

; Exception 6 — Invalid Opcode (#UD)
; Captura snapshot completo antes de llamar al dispatcher.
; Layout del stack después de pushState (15 registros × 8 bytes):
;   [rsp +   0] = R15
;   [rsp +   8] = R14
;   [rsp +  16] = R13
;   [rsp +  24] = R12
;   [rsp +  32] = R11
;   [rsp +  40] = R10
;   [rsp +  48] = R9
;   [rsp +  56] = R8
;   [rsp +  64] = RBP
;   [rsp +  72] = RDI
;   [rsp +  80] = RSI
;   [rsp +  88] = RDX
;   [rsp +  96] = RCX
;   [rsp + 104] = RBX
;   [rsp + 112] = RAX
; Interrupt frame (puesto por el CPU antes del handler):
;   [rsp + 120] = RIP      ← dirección de la instrucción inválida
;   [rsp + 128] = CS
;   [rsp + 136] = RFLAGS
;   [rsp + 144] = RSP      ← RSP del proceso interrumpido
;   [rsp + 152] = SS
_exception6Handler:
    pushState

    ; --- Capturar registros de propósito general ---
    mov rax, [rsp + 112]
    mov [reg_snapshot + 0*8], rax    ; RAX

    mov rax, [rsp + 104]
    mov [reg_snapshot + 1*8], rax    ; RBX

    mov rax, [rsp + 96]
    mov [reg_snapshot + 2*8], rax    ; RCX

    mov rax, [rsp + 88]
    mov [reg_snapshot + 3*8], rax    ; RDX

    mov rax, [rsp + 80]
    mov [reg_snapshot + 4*8], rax    ; RSI

    mov rax, [rsp + 72]
    mov [reg_snapshot + 5*8], rax    ; RDI

    mov rax, [rsp + 64]
    mov [reg_snapshot + 6*8], rax    ; RBP

    ; RSP: tomamos el del interrupt frame (RSP del proceso interrumpido)
    mov rax, [rsp + 144]
    mov [reg_snapshot + 7*8], rax    ; RSP

    mov rax, [rsp + 56]
    mov [reg_snapshot + 8*8], rax    ; R8

    mov rax, [rsp + 48]
    mov [reg_snapshot + 9*8], rax    ; R9

    mov rax, [rsp + 40]
    mov [reg_snapshot + 10*8], rax   ; R10

    mov rax, [rsp + 32]
    mov [reg_snapshot + 11*8], rax   ; R11

    mov rax, [rsp + 24]
    mov [reg_snapshot + 12*8], rax   ; R12

    mov rax, [rsp + 16]
    mov [reg_snapshot + 13*8], rax   ; R13

    mov rax, [rsp + 8]
    mov [reg_snapshot + 14*8], rax   ; R14

    mov rax, [rsp + 0]
    mov [reg_snapshot + 15*8], rax   ; R15

    ; --- Capturar RIP y RFLAGS del interrupt frame ---
    mov rax, [rsp + 120]
    mov [reg_snapshot + 16*8], rax   ; RIP  ← instrucción que causó #UD

    mov rax, [rsp + 136]
    mov [reg_snapshot + 17*8], rax   ; RFLAGS

    ; Marcar snapshot disponible
    mov byte [snapshot_available], 1

    ; Llamar al dispatcher con exception=6
    mov rdi, 6
    mov rsi, rsp
    call exceptionDispatcher

    popState
    iretq