#include <stdint.h>
#include "./usrlib/usrlib.h"

int main(void) {
    clearScreen();

    // 1. Probá drawChar para re-confirmar que funciona en aislamiento
    drawChar('A', 0xFFFFFF);
    newLine();

    // 2. Construimos un string manualmente en el stack
    char mi_texto[5];
    mi_texto[0] = 'H';
    mi_texto[1] = 'o';
    mi_texto[2] = 'l';
    mi_texto[3] = 'a';
    mi_texto[4] = '\0'; // No olvides el terminador

    // 3. Llamá a tu función drawString pasándole este array local
    drawString(mi_texto, 0xFFFFFF);

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
