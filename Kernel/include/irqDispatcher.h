#ifndef IRQDISPATCHER_H
#define IRQDISPATCHER_H

#include <stdint.h>

void irqDispatcher(uint64_t irq);
void register_irq_handler(int irq, void (*handler)(void));

#endif
