GLOBAL atomic_xchg
GLOBAL asm_cli
GLOBAL asm_hlt
; asm_sti está en loader.asm

section .text

; uint64_t atomic_xchg(uint64_t *addr, uint64_t val)
;   rdi = addr, rsi = val
;   Intercambia atómicamente *addr con val y retorna el valor previo de *addr.
;   La instrucción xchg con operando de memoria implica LOCK implícito.
atomic_xchg:
	mov rax, rsi
	xchg [rdi], rax
	ret

; void asm_cli(void) — deshabilita interrupciones (clear IF)
asm_cli:
	cli
	ret

; void asm_hlt(void) — hlt hasta la próxima interrupción
asm_hlt:
	hlt
	ret
