#include <stdint.h>
#include <stdarg.h>
#include "usrlib.h"

/* ----------------------------------------------------------------
 * Funciones existentes (sin cambios)
 * ---------------------------------------------------------------- */

void fillRectangle(int x, int y, int width, int height, uint32_t color)
{
    uint64_t info[] = {x, y, width, height, color};
    sys_fillRectangle(info);
}

int strcmp(const char *str1, const char *str2)
{
    while (*str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
    }
    return (unsigned char)(*str1) - (unsigned char)(*str2);
}

void shell_print(char *str, uint32_t color)
{
    drawString(str, color);
    newLine();
}

void getTimeString(char *buffer)
{
    uint64_t h = getHours();
    uint64_t m = getMinutes();
    uint64_t s = getSeconds();
    buffer[0] = (h / 10) + '0';
    buffer[1] = (h % 10) + '0';
    buffer[2] = ':';
    buffer[3] = (m / 10) + '0';
    buffer[4] = (m % 10) + '0';
    buffer[5] = ':';
    buffer[6] = (s / 10) + '0';
    buffer[7] = (s % 10) + '0';
    buffer[8] = '\0';
}

void getDateString(char *buffer)
{
    uint64_t d = getDay();
    uint64_t m = getMonth();
    uint64_t y = getYear();
    buffer[0] = (d / 10) + '0';
    buffer[1] = (d % 10) + '0';
    buffer[2] = '/';
    buffer[3] = (m / 10) + '0';
    buffer[4] = (m % 10) + '0';
    buffer[5] = '/';
    buffer[6] = ((y / 1000) % 10) + '0';
    buffer[7] = ((y / 100) % 10) + '0';
    buffer[8] = ((y / 10) % 10) + '0';
    buffer[9] = (y % 10) + '0';
    buffer[10] = '\0';
}

void uint64ToHex(uint64_t value, char *buffer)
{
    buffer[0] = '0';
    buffer[1] = 'x';
    for (int i = 15; i >= 0; i--)
    {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        buffer[17 - i] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
    }
    buffer[18] = '\0';
}

void intToString(int value, char *buffer)
{
    if (value == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    int neg = (value < 0);
    if (neg)
        value = -value;
    char tmp[12];
    int i = 0;
    while (value > 0)
    {
        tmp[i++] = '0' + (value % 10);
        value /= 10;
    }
    int j = 0;
    if (neg)
        buffer[j++] = '-';
    while (i > 0)
        buffer[j++] = tmp[--i];
    buffer[j] = '\0';
}

/* ----------------------------------------------------------------
 * printf mínimo (F4)
 *
 * Soporta: %d (int), %u (unsigned), %x (hex sin prefijo), %s, %c, %%.
 * Color fijo blanco (0xFFFFFF) — suficiente para los tests de F4.
 * Para texto con color usar drawString directamente.
 * ---------------------------------------------------------------- */

static void _puts(const char *s)
{
    if (!s)
        s = "(null)";
    drawString(s, 0xFFFFFF);
}

static void _putc(char c)
{
    if (c == '\n')
        newLine();
    else
        drawChar((uint64_t)(unsigned char)c, 0xFFFFFF);
}

static void _print_uint(uint64_t v, int base)
{
    char buf[21];
    int i = 0;
    if (v == 0)
    {
        _putc('0');
        return;
    }
    while (v > 0)
    {
        unsigned d = (unsigned)(v % (uint64_t)base);
        buf[i++] = (d < 10) ? ('0' + d) : ('a' + d - 10);
        v /= (uint64_t)base;
    }
    while (--i >= 0)
        _putc(buf[i]);
}

static void _print_int(int64_t v)
{
    if (v < 0)
    {
        _putc('-');
        v = -v;
    }
    _print_uint((uint64_t)v, 10);
}

static void _putcA(char c)
{
    if (c == '\n')
        newLine();
    else
        drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
}

// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }
// static void _putcB(char c)
// {
//     if (c == '\n')
//         newLine();
//     else
//         drawChar((uint64_t)(unsigned char)c, 0xFFFFFA);
// }