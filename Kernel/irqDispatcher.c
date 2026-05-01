#include "irqDispatcher.h"
#include "time.h"
#include "keyboardDriver.h"

extern uint8_t pressed_key;

void irqDispatcher(uint64_t irq) {
    switch (irq) {
        case 0:  // Timer (IRQ0)
            timeHandler();
            break;
        case 1:  // Teclado — scancode leído por _irq01Handler
            kbd_handler(pressed_key);
            break;
        default:
            break;
    }
}
