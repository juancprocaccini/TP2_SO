#include <stdint.h>
#include "usrlib.h"
#include "apps.h"

/* Estado compartido — todo userland corre en el mismo address space */
static volatile char mvar_shared = 0;
static int           mvar_sem_empty = -1;
static int           mvar_sem_full  = -1;

/* Estado RNG en el stack del hijo: cada proceso avanza su propia secuencia. */
static void busy_wait_random(uint32_t *rng) {
    *rng = *rng * 1103515245u + 12345u;
    uint32_t iters = (*rng >> 16) % 32768u + 8000u; /* 8000..40767 */
    volatile uint32_t x = 0;
    for (uint32_t i = 0; i < iters; i++) x++;
}

static int mvar_writer(char **argv, int argc) {
    if (argc < 2) return 1;
    int ok;
    int idx = satoi(argv[1], &ok);
    if (!ok) return 1;
    char letra = (char)('A' + idx);

    uint32_t rng = (uint32_t)getpid() * 2654435761u + 1u;
    while (1) {
        busy_wait_random(&rng);
        sem_wait(mvar_sem_empty);
        mvar_shared = letra;
        sem_post(mvar_sem_full);
    }
    return 0;
}

static int mvar_reader(char **argv, int argc) {
    (void)argv; (void)argc;
    uint32_t rng = (uint32_t)getpid() * 2654435761u + 1u;
    while (1) {
        busy_wait_random(&rng);
        sem_wait(mvar_sem_full);
        char c = mvar_shared;
        sem_post(mvar_sem_empty);
        putchar(c);
    }
    return 0;
}

int cmd_mvar(char **argv, int argc) {
    if (argc < 3) {
        printf("Uso: mvar <W> <R>\n");
        return 1;
    }
    int ok;
    int W = satoi(argv[1], &ok);
    if (!ok || W <= 0) {
        printf("mvar: W invalido\n");
        return 1;
    }
    int R = satoi(argv[2], &ok);
    if (!ok || R <= 0) {
        printf("mvar: R invalido\n");
        return 1;
    }

    mvar_sem_empty = sem_open_get_id(1);
    mvar_sem_full  = sem_open_get_id(0);
    if (mvar_sem_empty < 0 || mvar_sem_full < 0) {
        printf("mvar: no se pudo abrir semaforos\n");
        return 1;
    }

    int fds[3] = {0, 1, 2};
    char idx_buf[12];

    for (int i = 0; i < W; i++) {
        intToString(i, idx_buf);
        char *wargv[] = {"mvar_w", idx_buf, 0};
        int pid = create_process(mvar_writer, U_MEDIUM, 1, wargv, 2, fds);
        if (pid < 0) {
            printf("mvar: error al crear escritor %d\n", i);
            return 1;
        }
    }

    for (int i = 0; i < R; i++) {
        intToString(i, idx_buf);
        char *rargv[] = {"mvar_r", idx_buf, 0};
        int pid = create_process(mvar_reader, U_MEDIUM, 1, rargv, 2, fds);
        if (pid < 0) {
            printf("mvar: error al crear lector %d\n", i);
            return 1;
        }
    }

    /* El proceso principal termina de inmediato (consigna) */
    return 0;
}
