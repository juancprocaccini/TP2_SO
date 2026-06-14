#include "usrlib.h"
#include "apps.h"

int cmd_wc(char **argv, int argc) {
    (void)argv; (void)argc;
    int c, lines = 0, in_line = 0;
    while ((c = getchar()) != 0) {
        if (c == '\n') {
            lines++;
            in_line = 0;
        } else {
            in_line = 1;
        }
    }
    /* Última línea sin '\n' final también cuenta */
    if (in_line)
        lines++;
    printf("%d\n", lines);
    return 0;
}
