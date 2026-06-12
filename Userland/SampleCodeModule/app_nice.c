#include <stdint.h>
#include "usrlib.h"
#include "apps.h"

int cmd_nice(char **argv, int argc) {
    if (argc < 3) {
        printf("Uso: nice <pid> <low|medium|high>\n");
        return 1;
    }
    int ok;
    int pid = satoi(argv[1], &ok);
    if (!ok) {
        printf("nice: pid invalido\n");
        return 1;
    }
    int prio;
    if (strncasecmp(argv[2], "low", 3) == 0 && argv[2][3] == '\0')
        prio = U_LOW;
    else if (strncasecmp(argv[2], "medium", 6) == 0 && argv[2][6] == '\0')
        prio = U_MEDIUM;
    else if (strncasecmp(argv[2], "high", 4) == 0 && argv[2][4] == '\0')
        prio = U_HIGH;
    else {
        printf("nice: prioridad invalida (low/medium/high)\n");
        return 1;
    }
    int r = nice(pid, prio);
    if (r < 0)
        printf("nice: error\n");
    return r < 0 ? 1 : 0;
}
