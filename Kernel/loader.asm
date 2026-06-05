global loader
global outb
global inb
global io_wait
global _sti

extern main
extern initializeKernelBinary

loader:
	call initializeKernelBinary	; Set up the kernel binary, and get thet stack address
	mov rsp, rax				; Set up the stack with the returned address
	call main
hang:
	cli
	hlt	; halt machine should kernel return
	jmp hang

; Function: outb
; Writes a byte to the specified port
outb:
    mov dx, di          ; Port in dx
    mov al, sil         ; Value in al
    out dx, al
    ret

; Function: inb
; Reads a byte from the specified port
inb:
    mov dx, di          ; Port in dx
    in al, dx           ; Read from port
    ret

; Function: io_wait
; Introduces a small delay
io_wait:
    mov al, 0
    out 0x80, al
    ret

; Function: asm_sti
; Enables global interrupts
_sti:
    sti                 ; Enable interrupts
    ret
