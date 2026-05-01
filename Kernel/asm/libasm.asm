GLOBAL load_idt
GLOBAL setTimerFreq
GLOBAL getSeconds
GLOBAL getMinutes
GLOBAL getHours
GLOBAL getDay
GLOBAL getMonth
GLOBAL getYear
GLOBAL picMasterMask

section .text

load_idt:
	push rbp
	mov rbp, rsp

	; En idtLoader.c tenemos el array idt[256]
	; Necesitamos cargar el IDTR
	sub rsp, 10

	; Dirección base del IDT (idt array externo)
	extern idt
	mov rax, idt
	mov [rsp+2], rax

	; Límite (256 entradas * 16 bytes cada una - 1)
	mov word [rsp], 256*16 - 1

	lidt [rsp]

	add rsp, 10
	mov rsp, rbp
	pop rbp
	ret

setTimerFreq:
	push rbp
	mov rbp, rsp

	; Frecuencia del PIT: 1193182 Hz
	; Divisor = 1193182 / frecuencia deseada
	mov rax, 1193182
	xor rdx, rdx
	div rdi

	mov rcx, rax

	; Configurar el PIT — modo 3 (square wave), canal 0, acceso lobyte/hibyte
	mov al, 0x36
	out 0x43, al

	; Enviar divisor (low byte primero, luego high byte)
	mov ax, cx
	out 0x40, al
	mov al, ah
	out 0x40, al

	mov rsp, rbp
	pop rbp
	ret

; === Funciones RTC (Real Time Clock) ===
; Puertos: 0x70 (índice de registro), 0x71 (datos)
; Los valores están en BCD y se convierten a binario.

getSeconds:
	push rbp
	mov rbp, rsp

	mov al, 0x00        ; Registro de segundos
	out 0x70, al
	in al, 0x71

	; Convertir de BCD a binario
	mov bl, al
	shr al, 4
	mov cl, 10
	mul cl
	and bl, 0x0F
	add al, bl

	movzx rax, al

	mov rsp, rbp
	pop rbp
	ret

getMinutes:
	push rbp
	mov rbp, rsp

	mov al, 0x02        ; Registro de minutos
	out 0x70, al
	in al, 0x71

	; Convertir de BCD a binario
	mov bl, al
	shr al, 4
	mov cl, 10
	mul cl
	and bl, 0x0F
	add al, bl

	movzx rax, al

	mov rsp, rbp
	pop rbp
	ret

getHours:
	push rbp
	mov rbp, rsp

	mov al, 0x04        ; Registro de horas
	out 0x70, al
	in al, 0x71

	; Convertir de BCD a binario
	mov bl, al
	shr al, 4
	mov cl, 10
	mul cl
	and bl, 0x0F
	add al, bl

	; Ajustar zona horaria (UTC-3 para Argentina)
	sub al, 3
	cmp al, 0
	jge .no_wrap
	add al, 24
.no_wrap:

	movzx rax, al

	mov rsp, rbp
	pop rbp
	ret

getDay:
	push rbp
	mov rbp, rsp

	mov al, 0x07        ; Registro de día del mes
	out 0x70, al
	in al, 0x71

	; Convertir de BCD a binario
	mov bl, al
	shr al, 4
	mov cl, 10
	mul cl
	and bl, 0x0F
	add al, bl

	movzx rax, al

	mov rsp, rbp
	pop rbp
	ret

getMonth:
	push rbp
	mov rbp, rsp

	mov al, 0x08        ; Registro de mes
	out 0x70, al
	in al, 0x71

	; Convertir de BCD a binario
	mov bl, al
	shr al, 4
	mov cl, 10
	mul cl
	and bl, 0x0F
	add al, bl

	movzx rax, al

	mov rsp, rbp
	pop rbp
	ret

getYear:
	push rbp
	mov rbp, rsp

	mov al, 0x09        ; Registro de año (últimos 2 dígitos)
	out 0x70, al
	in al, 0x71

	; Convertir de BCD a binario
	mov bl, al
	shr al, 4
	mov cl, 10
	mul cl
	and bl, 0x0F
	add al, bl

	movzx rax, al
	add rax, 2000       ; Convertir a año completo (20xx)

	mov rsp, rbp
	pop rbp
	ret

; === Funciones PIC (Programmable Interrupt Controller) ===
picMasterMask:
	push rbp
	mov rbp, rsp

	mov ax, di          ; Máscara en di (primer argumento)
	out 0x21, al        ; Puerto 0x21 = PIC maestro data

	pop rbp
	ret
