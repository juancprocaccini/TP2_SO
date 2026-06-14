#include "usrlib.h"
#include "apps.h"

/* Retorna 1 si c es vocal (mayúscula o minúscula). */
static int es_vocal(char c) {
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
           c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
}

int cmd_filter(char **argv, int argc) {
    (void)argv; (void)argc;
    int c;
    while ((c = getchar()) != 0) {
        if (!es_vocal((char)c))
            putchar((char)c);
    }
    return 0;
}
