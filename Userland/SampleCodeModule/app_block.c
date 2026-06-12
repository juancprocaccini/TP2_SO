#include <stdint.h>
#include "usrlib.h"
#include "apps.h"

int cmd_block(char **argv, int argc) {
    if (argc < 2) {
        printf("Uso: block <pid>\n");
        return 1;
    }
    int ok;
    int pid = satoi(argv[1], &ok);
    if (!ok) {
        printf("block: pid invalido\n");
        return 1;
    }
    int st = get_status(pid);
    if (st < 0) {
        printf("block: proceso no encontrado\n");
        return 1;
    }
    int r;
    if (st == U_BLOCKED)
        r = unblock(pid);
    else
        r = block(pid);
    if (r < 0)
        printf("block: error\n");
    return r < 0 ? 1 : 0;
}
