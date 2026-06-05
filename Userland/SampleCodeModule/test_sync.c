#include "usrlib.h"
#include <stddef.h>

static int shared_var = 0;

static int my_atoi(const char *str)
{
    int res = 0;
    int sign = 1;
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    while (*str >= '0' && *str <= '9')
    {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res * sign;
}

int inc_process(int argc, char **argv)
{
    if (argc < 3)
        return -1;

    int iters = my_atoi(argv[1]);
    int use_sem = my_atoi(argv[2]);
    int sem_id = 1;

    for (int i = 0; i < iters; i++)
    {
        if (use_sem)
            sem_wait(sem_id); 

        int temp = shared_var;
        yield();
        shared_var = temp + 1;

        if (use_sem)
            sem_post(sem_id); 
    }
    return 0;
}

int dec_process(int argc, char **argv)
{
    if (argc < 3)
        return -1;

    int iters = my_atoi(argv[1]);
    int use_sem = my_atoi(argv[2]);
    int sem_id = 1;

    for (int i = 0; i < iters; i++)
    {
        if (use_sem)
            sem_wait(sem_id);

        int temp = shared_var;
        yield();
        shared_var = temp - 1;

        if (use_sem)
            sem_post(sem_id);
    }
    return 0;
}

void test_sync(int pairs, int iterations, int use_sem)
{
    shared_var = 0;
    int sem_id = 1;

    shell_print("==============================", 0xFFFFFF);
    if (use_sem)
    {
        shell_print("MODO: CON Semaforos", 0x00FF00);
        sem_open(sem_id, 1);
    }
    else
    {
        shell_print("MODO: SIN Semaforos", 0xFF0000);
    }

    int pids[100];

    char iters_str[16];
    char use_sem_str[2];
    intToString(iterations, iters_str);
    intToString(use_sem, use_sem_str);

    char *argv_inc[] = {"inc_worker", iters_str, use_sem_str};
    char *argv_dec[] = {"dec_worker", iters_str, use_sem_str};

    for (int i = 0; i < pairs; i++)
    {
        pids[i * 2] = create_process((void *)inc_process, 1, 1, argv_inc, 3, NULL);
        pids[i * 2 + 1] = create_process((void *)dec_process, 1, 1, argv_dec, 3, NULL);
    }

    shell_print("Esperando procesos...", 0xFFFFFF);

    for (int i = 0; i < pairs * 2; i++)
    {
        int status;
        waitpid(pids[i], &status);
    }

    if (use_sem)
    {
        sem_close(sem_id);
    }

    shell_print("Valor final:", 0xFFFFFF);
    char result_str[16];
    intToString(shared_var, result_str);

    shell_print(result_str, (shared_var == 0) ? 0x00FF00 : 0xFF0000);
    shell_print("==============================", 0xFFFFFF);
}