#include "time.h"

static uint64_t ticks = 0;

void timeHandler(void) {
    ticks++;
}

uint64_t getTicks(void) {
    return ticks;
}

int secondsPassed(void) {
    return ticks / 100;
}

void getDate(uint8_t* buffer) {
    buffer[0] = (uint8_t)getDay();
    buffer[1] = (uint8_t)getMonth();
    buffer[2] = (uint8_t)(getYear() & 0xFF);
    buffer[3] = (uint8_t)((getYear() >> 8) & 0xFF);
}

void getTime(uint8_t* buffer) {
    buffer[0] = (uint8_t)getHours();
    buffer[1] = (uint8_t)getMinutes();
    buffer[2] = (uint8_t)getSeconds();
}

void timer_wait(int seconds) {
    uint64_t inicio = getSeconds();
    while ((getSeconds() - inicio) < (uint64_t)seconds);
}
