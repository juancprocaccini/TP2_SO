#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include "process.h"

// Bits de scancode: indica si la tecla fue presionada o soltada
#define BIT_SCANCODE_UP   0b10000000
#define BIT_SCANCODE_DOWN 0b00000000

// Puertos del controlador de teclado PS/2
#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_COMMAND_PORT 0x64

// Macros para interpretar scancodes
#define SCANCODE_RELEASED   0x80
#define IS_KEY_RELEASED(sc) ((sc) & SCANCODE_RELEASED)
#define GET_KEY_CODE(sc)    ((sc) & 0x7F)

// Inicializa el driver de teclado
void kbd_init(void);

// Rutina de atención a la interrupción del teclado (IRQ1)
void kbd_handler(uint8_t scancode);

// Operaciones sobre el buffer circular de caracteres
char     kbd_buffer_get(void);
uint32_t kbd_read_chars(char* buffer, uint32_t max_chars);
uint32_t kbd_available_chars(void);
void     kbd_clear_buffer(void);

// Lectura bloqueante (F7): bloquea el proceso hasta que haya input
int stdin_read(char *buf, int n);

// Anula kbd_waiting_pcb si apunta a p (llamar antes de liberar el PCB).
void kbd_clear_waiter(PCB *p);

#endif
