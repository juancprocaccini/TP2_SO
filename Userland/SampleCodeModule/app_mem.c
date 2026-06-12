#include <stdint.h>
#include "usrlib.h"
#include "apps.h"

int cmd_mem(char **argv, int argc) {
    (void)argv; (void)argc;
    MemStats stats[2];
    sys_mem_state(stats);

    printf("=== KERNEL HEAP ===\n");
    printf("  Total: %d bytes\n", (int)stats[0].total);
    printf("  Used:  %d bytes\n", (int)stats[0].used);
    printf("  Free:  %d bytes\n", (int)stats[0].free);
    printf("=== USER HEAP ===\n");
    printf("  Total: %d bytes\n", (int)stats[1].total);
    printf("  Used:  %d bytes\n", (int)stats[1].used);
    printf("  Free:  %d bytes\n", (int)stats[1].free);
    return 0;
}
