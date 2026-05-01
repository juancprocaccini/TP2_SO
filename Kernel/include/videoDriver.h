#ifndef VIDEODRIVER_H
#define VIDEODRIVER_H

#include <stdint.h>

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16
#define NCHARS      95

// Operaciones de píxel/rectángulo
void putPixel(uint64_t hexColor, uint64_t x, uint64_t y);
void fillRectangle(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint64_t color);

// Operaciones de texto
void clearScreen(void);
void newLine(void);
void scrollDown(void);
void moveCursorLeft(void);
void moveCursorRight(void);
void setCursorPosition(uint32_t x, uint32_t y);
void deleteChar(void);

// Dibujo de caracteres y cadenas
void vPutChar(uint64_t c, uint64_t color);
void vprintString(const char* s, uint64_t color);
void vd_drawChar(uint64_t x, uint64_t y, char ch, uint64_t color, uint64_t size);
void vd_drawString(uint64_t x, uint64_t y, const char* s, uint64_t color, uint64_t size);
void vd_drawIntAt(int x, int y, int val, uint64_t color, uint64_t size, int right_align);

// Control de tamaño de fuente
void increaseFontSize(void);
void decreaseFontSize(void);
void redrawScreen(void);

// Resolución de pantalla
uint32_t getScreenWidth(void);
uint32_t getScreenHeight(void);

#endif // VIDEODRIVER_H
