#include "idtLoader.h"
#include "lib.h"
#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} DESCR_INT;

#pragma pack(pop)

DESCR_INT idt[256];

static void setup_IDT_entry(int index, uint64_t offset);

void load_idt();

void idtLoader() {
    // Excepciones del procesador (0-31)
    setup_IDT_entry(0x00, (uint64_t)&_exception0Handler);
    setup_IDT_entry(0x06, (uint64_t)&_exception6Handler);
    
    // IRQs - Timer, Teclado, etc (32-47)
    setup_IDT_entry(0x20, (uint64_t)&_irq00Handler);  // Timer
    setTimerFreq(11932);
    picMasterMask(0xFC); // Habilitar IRQ0 (timer) e IRQ1 (teclado)
    setup_IDT_entry(0x21, (uint64_t)&_irq01Handler);  // Teclado
    
    // Syscalls (INT 0x80)
    setup_IDT_entry(0x80, (uint64_t)&_syscallHandler);

 
    load_idt();
}

static void setup_IDT_entry(int index, uint64_t offset) {
    idt[index].selector = 0x08;
    idt[index].offset_low = offset & 0xFFFF;
    idt[index].offset_middle = (offset >> 16) & 0xFFFF;
    idt[index].offset_high = (offset >> 32) & 0xFFFFFFFF;
    idt[index].ist = 0;
    idt[index].type_attr = 0x8E;  // P=1, DPL=00, Type=1110 (Interrupt Gate)
    idt[index].reserved = 0;
}
