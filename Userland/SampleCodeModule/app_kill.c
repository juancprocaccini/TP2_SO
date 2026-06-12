#include <stdint.h>
#include "usrlib.h"
#include "apps.h"

int cmd_kill(char **argv, int argc) {
    if (argc < 2) {
        printf("Uso: kill <pid>\n");
        return 1;
    }
    int ok;
    int pid = satoi(argv[1], &ok);
    if (!ok) {
        printf("kill: pid invalido\n");
        return 1;
    }
    int r = kill(pid);
    if (r < 0)
        printf("kill: error al matar pid %d\n", pid);
    return r < 0 ? 1 : 0;
}
