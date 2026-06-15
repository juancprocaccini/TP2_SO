#include "usrlib.h"
#include "test_util.h"

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

int test_processes(char *argv[], int argc)
{
    uint8_t rq;
    uint8_t alive = 0;
    uint8_t action;
    uint64_t max_processes;
    char *argvAux[] = {0};
    int fds[3] = {0, 1, 2};

    if (argc != 1)
        return -1;

    if ((max_processes = satoi(argv[0])) <= 0)
        return -1;

    p_rq p_rqs[max_processes];

    while (1)
    {
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
                alive++;
            }
        }

        while (alive > 0)
        {
            for (rq = 0; rq < max_processes; rq++)
            {
                action = GetUniform(100) % 2;

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
                if (p_rqs[rq].state == BLOCKED && (GetUniform(100) % 2))
                {
                    if (unblock(p_rqs[rq].pid) == -1)
                    {
                        printf("test_processes: ERROR unblocking process\n");
                        return -1;
                    }
                    p_rqs[rq].state = RUNNING;
                }
        }
    }
    return 0;
}