GLOBAL _irq00Handler
GLOBAL _irq01Handler
GLOBAL _syscallHandler
GLOBAL reg_snapshot
GLOBAL snapshot_available

EXTERN irqDispatcher
EXTERN syscallDispatcher

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

; IRQ handlers
_irq00Handler:
    pushState
    mov rdi, 0      ; IRQ number (Timer)
    call irqDispatcher
    mov al, 0x20
    out 0x20, al    ; EOI al Master PIC
    popState
    iretq

_irq01Handler:
    ; PRIMERO: Guardar todos los registros en el stack
    pushState
    
    ; SEGUNDO: Leer la tecla PRIMERO
    xor rax, rax
    in al, 0x60              ; Leer scancode del teclado
    mov [pressed_key], rax
    
    ; TERCERO: Si NO es F10 (scancode 0x44), saltar toda la captura
    cmp al, 0x44
    jne .skipCapture
    
    ; CUARTO: Solo si ES F10, capturar el snapshot
    ; Capturar desde el stack (estado del programa ANTES de la interrupción)
    ; pushState guarda: rax, rbx, rcx, rdx, rsi, rdi, rbp, r8-r15
    ; Entonces [rsp+0] = r15, [rsp+8] = r14, ... [rsp+112] = rax
    
    ; R15 está en [rsp + 0]
    mov rax, [rsp + 0]
    mov [reg_snapshot + 15*8], rax   ; R15
    
    ; R14 está en [rsp + 8]
    mov rax, [rsp + 8]
    mov [reg_snapshot + 14*8], rax   ; R14
    
    ; R13 está en [rsp + 16]
    mov rax, [rsp + 16]
    mov [reg_snapshot + 13*8], rax   ; R13
    
    ; R12 está en [rsp + 24]
    mov rax, [rsp + 24]
    mov [reg_snapshot + 12*8], rax   ; R12
    
    ; R11 está en [rsp + 32]
    mov rax, [rsp + 32]
    mov [reg_snapshot + 11*8], rax   ; R11
    
    ; R10 está en [rsp + 40]
    mov rax, [rsp + 40]
    mov [reg_snapshot + 10*8], rax   ; R10
    
    ; R9 está en [rsp + 48]
    mov rax, [rsp + 48]
    mov [reg_snapshot + 9*8], rax    ; R9
    
    ; R8 está en [rsp + 56]
    mov rax, [rsp + 56]
    mov [reg_snapshot + 8*8], rax    ; R8
    
    ; RBP está en [rsp + 64]
    mov rax, [rsp + 64]
    mov [reg_snapshot + 6*8], rax    ; RBP
    
    ; RDI está en [rsp + 72]
    mov rax, [rsp + 72]
    mov [reg_snapshot + 5*8], rax    ; RDI
    
    ; RSI está en [rsp + 80]
    mov rax, [rsp + 80]
    mov [reg_snapshot + 4*8], rax    ; RSI
    
    ; RDX está en [rsp + 88]
    mov rax, [rsp + 88]
    mov [reg_snapshot + 3*8], rax    ; RDX
    
    ; RCX está en [rsp + 96]
    mov rax, [rsp + 96]
    mov [reg_snapshot + 2*8], rax    ; RCX
    
    ; RBX está en [rsp + 104]
    mov rax, [rsp + 104]
    mov [reg_snapshot + 1*8], rax    ; RBX
    
    ; RAX está en [rsp + 112]
    mov rax, [rsp + 112]
    mov [reg_snapshot + 0*8], rax    ; RAX
    
    ; En el interrupt frame (después de los 15 registros):
    ; [rsp + 120] = RIP
    ; [rsp + 128] = CS
    ; [rsp + 136] = RFLAGS
    ; [rsp + 144] = RSP (del usuario)
    ; [rsp + 152] = SS
    
    ; RIP del interrupt frame
    mov rax, [rsp + 120]
    mov [reg_snapshot + 16*8], rax   ; RIP
    
    ; RSP del interrupt frame
    mov rax, [rsp + 144]
    mov [reg_snapshot + 7*8], rax    ; RSP
    
    ; RFLAGS del interrupt frame
    mov rax, [rsp + 136]
    mov [reg_snapshot + 17*8], rax   ; RFLAGS
    
    ; Marcar snapshot como disponible
    mov byte [snapshot_available], 1
    
.skipCapture:
    ; Continuar con el handler normal
    mov rdi, 1      ; IRQ number (Keyboard)
    call irqDispatcher
    mov al, 0x20
    out 0x20, al    ; EOI al Master PIC
    popState
    iretq

; Syscall handler
_syscallHandler:
    push rbp
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
    
    ; Capturar snapshot de registros automáticamente para syscalls de excepción (25 y 26)
    ; Verificar si rax (syscall number) es 25 o 26
    cmp qword [rsp], 25
    je .captureSnapshot
    cmp qword [rsp], 26
    je .captureSnapshot
    jmp .skipAutoCapture
    
.captureSnapshot:
    ; Guardar el snapshot desde el stack
    ; RCX está en [rsp + 16]
    mov rax, [rsp + 16]
    mov [reg_snapshot + 2*8], rax    ; RCX
    
    ; RDX está en [rsp + 24]
    mov rax, [rsp + 24]
    mov [reg_snapshot + 3*8], rax    ; RDX
    
    ; RSI está en [rsp + 32]
    mov rax, [rsp + 32]
    mov [reg_snapshot + 4*8], rax    ; RSI
    
    ; RDI está en [rsp + 40]
    mov rax, [rsp + 40]
    mov [reg_snapshot + 5*8], rax    ; RDI
    
    ; R8 está en [rsp + 48]
    mov rax, [rsp + 48]
    mov [reg_snapshot + 8*8], rax    ; R8
    
    ; R9 está en [rsp + 56]
    mov rax, [rsp + 56]
    mov [reg_snapshot + 9*8], rax    ; R9
    
    ; R10 está en [rsp + 64]
    mov rax, [rsp + 64]
    mov [reg_snapshot + 10*8], rax   ; R10
    
    ; R11 está en [rsp + 72]
    mov rax, [rsp + 72]
    mov [reg_snapshot + 11*8], rax   ; R11
    
    ; R12 está en [rsp + 80]
    mov rax, [rsp + 80]
    mov [reg_snapshot + 12*8], rax   ; R12
    
    ; R13 está en [rsp + 88]
    mov rax, [rsp + 88]
    mov [reg_snapshot + 13*8], rax   ; R13
    
    ; R14 está en [rsp + 96]
    mov rax, [rsp + 96]
    mov [reg_snapshot + 14*8], rax   ; R14
    
    ; R15 está en [rsp + 104]
    mov rax, [rsp + 104]
    mov [reg_snapshot + 15*8], rax   ; R15
    
    ; RBP está en [rsp + 112]
    mov rax, [rsp + 112]
    mov [reg_snapshot + 6*8], rax    ; RBP
    
    ; RBX está en [rsp + 8]
    mov rax, [rsp + 8]
    mov [reg_snapshot + 1*8], rax    ; RBX
    
    ; RAX está en [rsp + 0]
    mov rax, [rsp + 0]
    mov [reg_snapshot + 0*8], rax    ; RAX
    
    ; En el interrupt frame (después de los 15 registros):
    ; [rsp + 120] = RIP
    ; [rsp + 128] = CS
    ; [rsp + 136] = RFLAGS
    ; [rsp + 144] = RSP (del usuario)
    
    ; RIP del interrupt frame
    mov rax, [rsp + 120]
    mov [reg_snapshot + 16*8], rax   ; RIP
    
    ; RSP del interrupt frame
    mov rax, [rsp + 144]
    mov [reg_snapshot + 7*8], rax    ; RSP
    
    ; RFLAGS del interrupt frame
    mov rax, [rsp + 136]
    mov [reg_snapshot + 17*8], rax   ; RFLAGS
    
    ; Marcar snapshot como disponible
    mov byte [snapshot_available], 1
    
.skipAutoCapture:
    
    ; Ajustar argumentos para la convención de llamada C
    ; En el momento del INT 0x80:
    ;   rax = syscall number
    ;   rdi = arg1, rsi = arg2, rdx = arg3, rcx = arg4, r8 = arg5, r9 = arg6
    ; Pero ahora están en el stack, necesitamos leerlos desde ahí
    
    ; Stack layout después de los pushes:
    ; [rsp+0]  = rax (syscall number)
    ; [rsp+8]  = rbx
    ; [rsp+16] = rcx (arg4 original)
    ; [rsp+24] = rdx (arg3 original)
    ; [rsp+32] = rsi (arg2 original)
    ; [rsp+40] = rdi (arg1 original)
    ; [rsp+48] = r8  (arg5 original)
    ; [rsp+56] = r9  (arg6 original)
    
    mov rdi, [rsp+0]     ; syscall_num
    mov rsi, [rsp+40]    ; arg1 (rdi original)
    mov rdx, [rsp+32]    ; arg2 (rsi original)
    mov rcx, [rsp+24]    ; arg3 (rdx original)
    mov r8,  [rsp+16]    ; arg4 (rcx original)
    mov r9,  [rsp+48]    ; arg5 (r8 original)
    ; arg6 (r9 original) iría en el stack como 7mo parámetro si fuera necesario
    
    call syscallDispatcher
    
    ; El valor de retorno está en rax
    mov [rsp], rax
    
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    
    iretq

; Función para capturar registros desde el stack del syscall actual
; Parámetro: rdi = puntero al buffer de usuario
; Los registros están guardados en el stack por _syscallHandler
capture_registers:
    ; Guardar rdi en r10
    mov r10, rdi
    
    ; El stack actual tiene los registros guardados por _syscallHandler
    ; Desde la perspectiva de esta función:
    ; - Hay una return address en [rsp]
    ; - El frame de syscallDispatcher está arriba
    ; - Los registros están más arriba en el stack
    
    ; Usar rbp para encontrar el frame de _syscallHandler
    ; rbp actual apunta al frame de capture_registers
    ; [rbp] = rbp anterior (de syscallDispatcher)
    ; [[rbp]] = rbp anterior (de _syscallHandler)
    
    mov r11, [rbp]          ; rbp de syscallDispatcher
    mov r11, [r11]          ; rbp de _syscallHandler
    sub r11, 120            ; Los registros están antes de rbp guardado
    
    ; Ahora r11 apunta al rax guardado (primer registro pusheado)
    ; Stack layout: [rax][rbx][rcx][rdx][rsi][rdi][r8][r9][r10][r11][r12][r13][r14][r15][rbp]
    
    ; RAX
    mov rax, [r11]
    mov [r10 + 0*8], rax
    
    ; RBX
    mov rax, [r11 + 8]
    mov [r10 + 1*8], rax
    
    ; RCX
    mov rax, [r11 + 16]
    mov [r10 + 2*8], rax
    
    ; RDX
    mov rax, [r11 + 24]
    mov [r10 + 3*8], rax
    
    ; RSI
    mov rax, [r11 + 32]
    mov [r10 + 4*8], rax
    
    ; RDI
    mov rax, [r11 + 40]
    mov [r10 + 5*8], rax
    
    ; RBP
    mov rax, [r11 + 112]
    mov [r10 + 6*8], rax
    
    ; RSP (calculado)
    lea rax, [r11 + 120]
    mov [r10 + 7*8], rax
    
    ; R8
    mov rax, [r11 + 48]
    mov [r10 + 8*8], rax
    
    ; R9
    mov rax, [r11 + 56]
    mov [r10 + 9*8], rax
    
    ; R10
    mov rax, [r11 + 64]
    mov [r10 + 10*8], rax
    
    ; R11
    mov rax, [r11 + 72]
    mov [r10 + 11*8], rax
    
    ; R12
    mov rax, [r11 + 80]
    mov [r10 + 12*8], rax
    
    ; R13
    mov rax, [r11 + 88]
    mov [r10 + 13*8], rax
    
    ; R14
    mov rax, [r11 + 96]
    mov [r10 + 14*8], rax
    
    ; R15
    mov rax, [r11 + 104]
    mov [r10 + 15*8], rax
    
    ; RIP (está después de todos los registros, es la return address del INT 0x80)
    mov rax, [r11 + 120]
    mov [r10 + 16*8], rax
    
    ; RFLAGS (está en el interrupt frame, después del RIP)
    mov rax, [r11 + 136]
    mov [r10 + 17*8], rax
    
    ret

SECTION .data
    snapshot_available db 0   ; Inicializado explícitamente en 0

SECTION .bss
    pressed_key resb 1
    reg_snapshot resq 18      ; Array para 18 registros (RAX-R15, RIP, RFLAGS)