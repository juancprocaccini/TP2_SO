#include "usrlib.h"
#include "test_util.h"

#define SEM_ID 1
#define TOTAL_PAIR_PROCESSES 2

int64_t global;

void slowInc(int64_t *p, int64_t inc)
{
    uint64_t aux = *p;
    if (GetUniform(100) < 30)
        yield();
    aux += inc;
    *p = aux;
}

int my_process_inc(char *argv[], int argc)
{
    uint64_t n;
    int8_t inc;
    int8_t use_sem;

    if (argc != 3)
        return -1;

    if ((n = satoi(argv[0])) <= 0)
        return -1;
    if ((inc = satoi(argv[1])) == 0)
        return -1;
    if ((use_sem = satoi(argv[2])) < 0)
        return -1;

    if (use_sem)
    {
        if (sem_open(SEM_ID, 1) < 0)
        {
            printf("test_sync: ERROR opening semaphore\n");
            return -1;
        }
    }

    uint64_t i;
    for (i = 0; i < n; i++)
    {
        if (use_sem)
            sem_wait(SEM_ID);
        slowInc(&global, inc);
        if (use_sem)
            sem_post(SEM_ID);
    }

    if (use_sem)
        sem_close(SEM_ID);

    return 0;
}

int test_sync(char *argv[], int argc)
{
    uint64_t pids[2 * TOTAL_PAIR_PROCESSES];
    int fds[3] = {0, 1, 2};

    if (argc != 2)
        return -1;

    char *argvDec[] = {argv[0], "-1", argv[1], 0};
    char *argvInc[] = {argv[0], "1", argv[1], 0};

    global = 0;

    uint64_t i;
    for (i = 0; i < TOTAL_PAIR_PROCESSES; i++)
    {
        pids[i] = create_process(my_process_inc, U_MEDIUM, 1, argvDec, 3, fds);
        pids[i + TOTAL_PAIR_PROCESSES] = create_process(my_process_inc, U_MEDIUM, 1, argvInc, 3, fds);
    }

    for (i = 0; i < TOTAL_PAIR_PROCESSES; i++)
    {
        waitpid(pids[i], 0);
        waitpid(pids[i + TOTAL_PAIR_PROCESSES], 0);
    }

    printf("Final value: %d\n", (int)global);

    return 0;
}