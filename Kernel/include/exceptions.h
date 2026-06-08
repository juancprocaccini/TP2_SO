#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdint.h>

void exceptionDispatcher(int exception, uint64_t rsp);
void uint64ToHexKernel(uint64_t value, char *buffer);
void uint8ToHexKernel(uint8_t value, char *buffer);

#endif
