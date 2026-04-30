#include "irqDispatcher.h"
#include "time.h"

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t value);
extern void io_wait(void);

extern void kbd_handler(uint8_t scancode);

static void (*irq_handlers[16])(void) = {0};

void irqDispatcher(uint64_t irq) {
    switch (irq) {
        case 0:  // Timer
        timeHandler();
            break;
        case 1:  // Keyboard
            {
                uint8_t scancode = inb(0x60);
                kbd_handler(scancode);
            }
            break;
        default:
            // IRQ no manejada
            break;
    }
    
   
}

void register_irq_handler(int irq, void (*handler)(void)) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = handler;
    }
}

