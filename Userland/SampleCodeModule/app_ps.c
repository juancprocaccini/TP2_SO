#include <stdint.h>
#include "usrlib.h"
#include "apps.h"

static const char *state_str(int s) {
    switch (s) {
        case 0: return "FREE";
        case 1: return "READY";
        case 2: return "BLOCKED";
        case 3: return "ZOMBIE";
        default: return "?";
    }
}

static const char *prio_str(int p) {
    switch (p) {
        case 0: return "LOW";
        case 1: return "MED";
        case 2: return "HIGH";
        default: return "?";
    }
}

int cmd_ps(char **argv, int argc) {
    (void)argv; (void)argc;
    ProcessInfoList *list = 0;
    int count = ps(&list);
    if (count < 0 || list == 0) {
        printf("ps: error\n");
        return 1;
    }

    printf("%-4s %-16s %-4s %-7s %-3s %2s %3s %3s  %s\n",
           "PID", "NAME", "PRIO", "STATE", "FG", "IN", "OUT", "ERR", "RSP");
    printf("%-4s %-16s %-4s %-7s %-3s %2s %3s %3s  %s\n",
           "---", "----------------", "----", "-------", "---", "--", "---", "---", "------------------");
    for (int i = 0; i < list->count; i++) {
        ProcessInfo *p = &list->entries[i];
        printf("%-4d %-16s %-4s %-7s %-3s %2d %3d %3d  %016x\n",
               p->pid,
               p->name,
               prio_str(p->priority),
               state_str(p->state),
               p->foreground ? "yes" : "no",
               p->fds[0], p->fds[1], p->fds[2],
               p->rsp);
    }

    free_ps(list);
    return 0;
}
