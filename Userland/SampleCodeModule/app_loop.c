#include <stdint.h>
#include "usrlib.h"
#include "apps.h"

int cmd_loop(char **argv, int argc) {
    if (argc < 2) {
        printf("Uso: loop <ticks>\n");
        return 1;
    }
    int ok;
    int ticks = satoi(argv[1], &ok);
    if (!ok || ticks <= 0) {
        printf("loop: argumento invalido\n");
        return 1;
    }
    volatile int x = 0;
    for (int i = 0; i < ticks; i++) x++;   /* espera activa (consigna) */
    printf("hello from pid %d\n", getpid());
    return 0;
}
