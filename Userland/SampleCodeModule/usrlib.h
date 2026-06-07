#ifndef USRLIB_H
#define USRLIB_H

#include <stdint.h>

/* --- Video --- */
extern void drawChar(uint64_t ch, uint64_t color);
extern void drawString(const char* s, uint64_t color);
extern void putPixel(uint64_t hexColor, uint64_t x, uint64_t y);
extern void sys_fillRectangle(uint64_t* info);
void fillRectangle(int x, int y, int width, int height, uint32_t color);
extern void clearScreen(void);
extern void newLine(void);
extern void scrollDown(void);
extern void moveCursorLeft(void);
extern void moveCursorRight(void);
extern void deleteChar(void);
extern void increaseFontSize(void);
extern void decreaseFontSize(void);
extern void vd_drawString(uint64_t x, uint64_t y, const char* s, uint64_t color, uint64_t size);
extern void vd_drawIntAt(int x, int y, int val, uint64_t color, uint64_t size, int right_align);
extern uint32_t getScreenWidth(void);
extern uint32_t getScreenHeight(void);

/* --- Teclado --- */
extern char kbdGetChar(void);

/* --- Tiempo / RTC --- */
extern uint64_t getSeconds(void);
extern uint64_t getMinutes(void);
extern uint64_t getHours(void);
extern uint64_t getDay(void);
extern uint64_t getMonth(void);
extern uint64_t getYear(void);
extern void getTime(uint8_t* buffer);
extern void secsToWait(int seconds);

/* --- Utilidades (usrlib.c) --- */
int strcmp(const char* str1, const char* str2);
void shell_print(char* str, uint32_t color);
void getTimeString(char* buffer);
void getDateString(char* buffer);
void intToString(int value, char* buffer);
void uint64ToHex(uint64_t value, char* buffer);

#endif
