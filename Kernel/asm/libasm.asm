GLOBAL cpuVendor
GLOBAL load_idt
GLOBAL setTimerFreq
GLOBAL getSeconds
GLOBAL getMinutes
GLOBAL getHours
GLOBAL getDay
GLOBAL getMonth
GLOBAL getYear
GLOBAL picMasterMask
GLOBAL picSlaveMask
global measure_keyboard_access
global measure_rtc_access

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
	
cpuVendor:
	push rbp
	mov rbp, rsp

	push rbx

	mov rax, 0
	cpuid


	mov [rdi], ebx
	mov [rdi + 4], edx
	mov [rdi + 8], ecx

	mov byte [rdi+13], 0

	mov rax, rdi

	pop rbx

	mov rsp, rbp
	pop rbp
	ret

setTimerFreq:
	push rbp
	mov rbp, rsp

	; La frecuencia del PIT es 1193182 Hz
	; Divisor = 1193182 / frecuencia deseada
	mov rax, 1193182
	xor rdx, rdx
	div rdi          ; rax = 1193182 / frecuencia (rdi tiene la frecuencia deseada)

	; Guardar el divisor
	mov rcx, rax     ; rcx = divisor

	; Configurar el PIT - Comando byte
	mov al, 0x36     ; Modo 3 (square wave), acceso lobyte/hibyte, canal 0
	out 0x43, al

	; Enviar el divisor (low byte primero, luego high byte)
	mov ax, cx       ; ax = divisor (parte baja de rcx)
	out 0x40, al     ; Enviar low byte
	mov al, ah       ; al = high byte del divisor
	out 0x40, al     ; Enviar high byte

	mov rsp, rbp
	pop rbp
	ret

; === Funciones RTC (Real Time Clock) ===
; El RTC usa los puertos 0x70 (comando) y 0x71 (datos)

getSeconds:
	push rbp
	mov rbp, rsp
	
	mov al, 0x00        ; Registro de segundos
	out 0x70, al        ; Seleccionar registro
	in al, 0x71         ; Leer valor
	
	; Convertir de BCD a binario
	mov bl, al          ; Guardar valor original
	shr al, 4           ; al = dígito alto (decenas)
	mov cl, 10
	mul cl              ; al = al * 10
	and bl, 0x0F        ; bl = dígito bajo (unidades)
	add al, bl          ; al = decenas*10 + unidades
	
	movzx rax, al       ; Extender a 64 bits
	
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
	sub al, 3           ; Restar 3 horas
	cmp al, 0
	jge .no_wrap        ; Si es positivo, no hay problema
	add al, 24          ; Si es negativo, sumar 24 (wrap around)
.no_wrap:
	
	movzx rax, al       ; Extender a 64 bits
	
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

; === Medición de acceso a RTC ===
measure_rtc_access:
	push rbp
	mov rbp, rsp
	push rbx
	
	; Leer TSC inicial
	rdtsc
	shl rdx, 32
	or rax, rdx          ; rax = TSC completo (64 bits)
	mov rbx, rax         ; rbx = inicio
	
	; Acceder al RTC (leer segundos)
	mov al, 0x00         ; Registro de segundos
	out 0x70, al         ; Seleccionar registro
	in al, 0x71          ; Leer valor del RTC
	
	; Leer TSC final
	rdtsc
	shl rdx, 32
	or rax, rdx          ; rax = TSC completo
	
	; Calcular diferencia
	sub rax, rbx         ; rax = ticks que tardó
	
	pop rbx
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

picSlaveMask:
	push rbp
	mov rbp, rsp
	
	mov ax, di          ; Máscara en di (primer argumento)
	out 0xA1, al        ; Puerto 0xA1 = PIC esclavo data
	
	pop rbp
	ret