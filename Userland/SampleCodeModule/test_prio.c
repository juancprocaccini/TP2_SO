#include "usrlib.h"
#include "test_util.h"

#define TOTAL_PROCESSES 3

#define LOWEST U_LOW
#define MEDIUM U_MEDIUM
#define HIGHEST U_HIGH

int64_t prio[TOTAL_PROCESSES] = {LOWEST, MEDIUM, HIGHEST};
uint64_t max_value = 0;

static int zero_to_max(char *argv[], int argc)
{
    uint64_t value = 0;
    while (value++ != max_value)
        ;
    printf("PROCESS %d DONE!\n", getpid());
    return 0;
}

int test_prio(char *argv[], int argc)
{
    int64_t pids[TOTAL_PROCESSES];
    char *ztm_argv[] = {0};
    uint64_t i;
    int fds[3] = {0, 1, 2};

    if (argc != 1)
        return -1;

    if ((max_value = satoi(argv[0])) <= 0)
        return -1;

    printf("SAME PRIORITY...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++)
        pids[i] = create_process(zero_to_max, U_MEDIUM, 1, ztm_argv, 0, fds);

    for (i = 0; i < TOTAL_PROCESSES; i++)
        waitpid(pids[i], 0);

    printf("SAME PRIORITY, THEN CHANGE IT...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++)
    {
        pids[i] = create_process(zero_to_max, U_MEDIUM, 1, ztm_argv, 0, fds);
        nice(pids[i], prio[i]);
        printf("  PROCESS %d NEW PRIORITY: %d\n", (int)pids[i], (int)prio[i]);
    }

    for (i = 0; i < TOTAL_PROCESSES; i++)
        waitpid(pids[i], 0);

    printf("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++)
    {
        pids[i] = create_process(zero_to_max, U_MEDIUM, 1, ztm_argv, 0, fds);
        block(pids[i]);
        nice(pids[i], prio[i]);
        printf("  PROCESS %d NEW PRIORITY: %d\n", (int)pids[i], (int)prio[i]);
    }

    for (i = 0; i < TOTAL_PROCESSES; i++)
        unblock(pids[i]);

    for (i = 0; i < TOTAL_PROCESSES; i++)
        waitpid(pids[i], 0);

    return 0;
}