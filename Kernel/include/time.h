#ifndef TIME_H
#define TIME_H

#include <stdint.h>

// Funciones de ticks del timer
void timeHandler(void);
uint64_t getTicks(void);
int secondsPassed(void);
int secondsPassed2(uint64_t ticks);

void timeHandler();

// Funciones RTC - obtener fecha y hora actual
void getDate(uint8_t* buffer);
void getTime(uint8_t* buffer);

// Funciones individuales RTC
uint8_t getSeconds(void);
uint8_t getMinutes(void);
uint8_t getHours(void);
uint8_t getDay(void);
uint8_t getMonth(void);
uint16_t getYear(void);

//funcion para pausa en segs
void timer_wait(int ticks_to_wait);

#endif
