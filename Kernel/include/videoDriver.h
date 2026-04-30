#ifndef VIDEODRIVER_H
#define VIDEODRIVER_H

#include <stdint.h>

#define CHAR_WIDTH 8
#define CHAR_HEIGHT 16
#define NCHARS 95
// Constantes para usar en kernel.c
#define FONT_SIZE 1
#define FONT_COLOR 0xFFFFFF
#define LINE_SPACING  (CHAR_HEIGHT * FONT_SIZE)  // Espacio entre líneas
#define CHAR_SPACING  (CHAR_WIDTH * FONT_SIZE)   // Espacio entre caracteres
#define SCREEN_WIDTH 1024   // Valores típicos, se actualizan después
#define SCREEN_HEIGHT 768


void init_video(void);

// Low-level pixel/rectangle
void putPixel(uint64_t hexColor, uint64_t x, uint64_t y);
void fillRectangle(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint64_t color);

// Text-mode helpers
void clearScreen(void);
void newLine(void);
void scrollDown(void);
void moveCursorLeft(void);
void moveCursorRight(void);
void setCursorPosition(uint32_t x, uint32_t y);
void deleteChar(void);

// Character drawing
void vPutChar(uint64_t c, uint64_t color);
void vprintString(const char* s, uint64_t color);
void vd_drawChar(uint64_t x, uint64_t y, char ch, uint64_t color, uint64_t size);
void vd_drawString(uint64_t x, uint64_t y, const char* s, uint64_t color, uint64_t size);
void vd_drawIntAt(int x, int y, int val, uint64_t color, uint64_t size, int right_align);

// Font size control
void increaseFontSize(void);
void decreaseFontSize(void);
void redrawScreen(void);
void sys_fillRectangle(uint64_t* args);

// Screen resolution getters
uint32_t getScreenWidth(void);
uint32_t getScreenHeight(void);

#endif // VIDEODRIVER_H
