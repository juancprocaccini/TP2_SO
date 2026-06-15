#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdint.h>
#include "usrlib.h" // Apuesta directo a tu librería de Userland

#define printf kprintf

// Generador del número aleatorio (PRNG) libre de dependencias
static uint32_t m_z = 362436069;
static uint32_t m_w = 521288629;

static inline uint32_t GetUint()
{
    m_z = 36969 * (m_z & 65535) + (m_z >> 16);
    m_w = 18000 * (m_w & 65535) + (m_w >> 16);
    return (m_z << 16) + m_w;
}

static inline uint32_t GetUniform(uint32_t max)
{
    uint32_t u = GetUint();
    return (u + 1.0) * 2.328306435454494e-10 * max;
}

// Verificación de memoria limpia
static inline uint8_t memcheck(void *start, uint8_t value, uint32_t size)
{
    uint8_t *p = (uint8_t *)start;
    uint32_t i;

    for (i = 0; i < size; i++, p++)
        if (*p != value)
            return 0;

    return 1;
}

// Conversión de string a entero (para parsear argv)
static inline int64_t satoiOld(char *str)
{
    uint64_t i = 0;
    int64_t res = 0;
    int8_t sign = 1;

    if (!str)
        return 0;

    if (str[i] == '-')
    {
        i++;
        sign = -1;
    }

    for (; str[i] != '\0'; ++i)
    {
        if (str[i] < '0' || str[i] > '9')
            return 0;
        res = res * 10 + str[i] - '0';
    }

    return res * sign;
}

// Espera ocupada para simular carga de CPU
static inline void bussy_wait(uint64_t n)
{
    uint64_t i;
    for (i = 0; i < n; i++)
        ;
}

static inline void endless_loop()
{
    while (1)
        ;
}

// Adaptado: Eliminamos la dependencia de 'printf' usando tus herramientas nativas
static inline void endless_loop_print(uint64_t wait)
{
    int pid = getpid(); // Usando tu int getpid(void) de usrlib.h
    char buf[12];
    intToString(pid, buf); // Usando tu intToString de strlib.c

    while (1)
    {
        drawString(buf, 0xFFFFFF); // Imprime el PID en pantalla
        drawString(" ", 0xFFFFFF);
        bussy_wait(wait);
    }
}

#endif