#ifndef TIME_H
#define TIME_H

#include <stdint.h>

// Manejador de interrupción del timer (IRQ0)
void timeHandler(void);

// Contadores de tiempo basados en ticks del timer
uint64_t getTicks(void);
int      secondsPassed(void);

// Funciones de fecha/hora (RTC)
void getDate(uint8_t* buffer);
void getTime(uint8_t* buffer);

// Funciones individuales del RTC (implementadas en libasm.asm)
uint64_t getSeconds(void);
uint64_t getMinutes(void);
uint64_t getHours(void);
uint64_t getDay(void);
uint64_t getMonth(void);
uint64_t getYear(void);

// Pausa de N segundos (bloqueante, basada en RTC)
void timer_wait(int seconds);

#endif
