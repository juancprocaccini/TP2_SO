#ifndef IDTLOADER_H
#define IDTLOADER_H

#include <stdint.h>

// Inicializar la IDT
void idtLoader();

// Handlers de excepciones (definidos en asm)
void _exception0Handler();
void _exception6Handler();

// Handlers de IRQs (definidos en asm)
void _irq00Handler();
void _irq01Handler();

// Handler de syscalls (definido en asm)
void _syscallHandler();

// Máscara del PIC maestro (implementada en libasm.asm)
void picMasterMask(uint8_t mask);

#endif
