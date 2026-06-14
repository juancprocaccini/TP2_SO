#include "usrlib.h"
#include "apps.h"

int cmd_cat(char **argv, int argc) {
    (void)argv; (void)argc;
    int c;
    while ((c = getchar()) != 0)
        putchar((char)c);
    return 0;
}
