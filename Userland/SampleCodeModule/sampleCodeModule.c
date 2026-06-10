#include <stdint.h>
#include "usrlib.h"

void test_sync(int pairs, int iterations, int use_sem);

int main(void) {
    
    clearScreen();

    //Prueba 1: SIN semáforos (5 pares, 100 iteraciones, 0 = sin semaforos)
    test_sync(5, 100, 0);

    //Prueba 2: CON semáforos (5 pares, 100 iteraciones, 1 = con semaforos)
    test_sync(5, 100, 1);

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
