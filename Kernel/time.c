#include <stdint.h>
#include "lib.h"

// Declaraciones de funciones implementadas en libasm.asm
extern uint64_t getSeconds(void);
extern uint64_t getMinutes(void);
extern uint64_t getHours(void);
extern uint64_t getDay(void);
extern uint64_t getMonth(void);
extern uint64_t getYear(void);

static uint64_t ticks = 0;

void timeHandler() {
    ticks++;
}

uint64_t getTicks() {
    return ticks;
}

int secondsPassed(){
    return ticks / 100;
}

void getDate(uint8_t* buffer){
    buffer[0] = (uint8_t)getDay();
    buffer[1] = (uint8_t)getMonth();
    buffer[2] = (uint8_t)(getYear() & 0xFF);         
    buffer[3] = (uint8_t)((getYear() >> 8) & 0xFF);  
}

void getTime(uint8_t* buffer){
    buffer[0] = (uint8_t)getHours();
    buffer[1] = (uint8_t)getMinutes();
    buffer[2] = (uint8_t)getSeconds();
}

int secondsPassed2(uint64_t ticks) {
    return(int) (ticks / 100);
}

void timer_wait(int seconds) {
    uint64_t inicio=getSeconds();
    int i =0;
    while((getSeconds()-inicio)<seconds){
        i++;
    }
}