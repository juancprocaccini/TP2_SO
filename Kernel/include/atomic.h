#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>

/*
 * Primitivas atómicas y de control de interrupciones.
 * Implementadas en Kernel/asm/atomic.asm.
 */

/*
 * Intercambio atómico: setea *addr = val y retorna el valor previo de *addr.
 * Usado para spinlocks (xchg con LOCK implícito por operando de memoria).
 */
uint64_t atomic_xchg(uint64_t *addr, uint64_t val);

/* Control de interrupciones a nivel CPU */
void asm_cli(void);
void asm_sti(void);
void asm_hlt(void);

#endif
