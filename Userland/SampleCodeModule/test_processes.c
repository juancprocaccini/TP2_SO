#include "usrlib.h"

enum State
{
    RUNNING,
    BLOCKED,
    KILLED
};

typedef struct P_rq
{
    int32_t pid;
    enum State state;
} p_rq;

static int endless_loop_proc(char *argv[], int argc)
{
    while (1)
    {
        yield();
    }
    return 0;
}

/* * Reemplazo de GetUniform de test_util.h
 * Un generador pseudoaleatorio simple (LCG) para no depender de librerías externas.
 */
static uint64_t seed = 1;
static int get_uniform(int max)
{
    seed = seed * 1103515245 + 12345;
    uint32_t rand_val = (uint32_t)(seed / 65536) % 32768;
    return rand_val % max;
}

int test_processes(char *argv[], int argc)
{
    uint8_t rq;
    uint8_t alive = 0;
    uint8_t action;
    int max_processes;
    char *argvAux[] = {0};
    int fds[3] = {0, 1, 2};

    if (argc != 1){
        printf("cant argumentos distinta de 1\n");
        return -1;
    }
        

    /* Implementación del nuevo satoi seguro */
    int ok;
    max_processes = satoi(argv[0], &ok);

    if (!ok || max_processes <= 0 || max_processes > 25){
        printf("Cantidad maxima de procesos del test es 25\n");
        return -1;
    }

    p_rq p_rqs[max_processes];

    for (rq = 0; rq < max_processes; rq++)
    {
        p_rqs[rq].pid = create_process(endless_loop_proc, U_LOW, 1, argvAux, 0, fds);

        if (p_rqs[rq].pid == -1)
        {
            printf("test_processes: ERROR creating process\n");
            return -1;
        }
        else
        {
            p_rqs[rq].state = RUNNING;
            printf("test_processes: Proceso creado exitosamente\n");
            alive++;
        }
    }

    while (alive > 0)
    {
        for (rq = 0; rq < max_processes; rq++)
        {
            action = get_uniform(100) % 2;

            switch (action)
            {
            case 0:
                if (p_rqs[rq].state == RUNNING || p_rqs[rq].state == BLOCKED)
                {
                    if (kill(p_rqs[rq].pid) == -1)
                    {
                        printf("test_processes: ERROR killing process\n");
                        return -1;
                    }
                    p_rqs[rq].state = KILLED;
                    printf("test_processes: Proceso eliminado exitosamente\n");
                    waitpid(p_rqs[rq].pid, 0);
                    alive--;
                }
                break;

            case 1:
                if (p_rqs[rq].state == RUNNING)
                {
                    if (block(p_rqs[rq].pid) == -1)
                    {
                        printf("test_processes: ERROR blocking process\n");
                        return -1;
                    }
                    p_rqs[rq].state = BLOCKED;
                }
                break;
            }
        }

        for (rq = 0; rq < max_processes; rq++)
        {
            if (p_rqs[rq].state == BLOCKED && (get_uniform(100) % 2))
            {
                if (unblock(p_rqs[rq].pid) == -1)
                {
                    printf("test_processes: ERROR unblocking process\n");
                    return -1;
                }
                printf("test_processes: Proceso bloqueado exitosamente\n");
                p_rqs[rq].state = RUNNING;
            }
        }
    }
    return 0;
}