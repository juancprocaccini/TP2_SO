#include "usrlib.h"

int main(void) {
    clearScreen();
    drawString("TP2 SO - Base lista", 0x00FF00);
    newLine();
    drawString("Presiona teclas...", 0xFFFFFF);
    newLine();

    while (1) {
        char c = kbdGetChar();
        if (c == 0) continue;
        if (c == '\n') {
            newLine();
        } else if (c == '\b') {
            deleteChar();
        } else {
            drawChar((uint64_t)c, 0xFFFFFF);
    }
    }

    return 0;
}
