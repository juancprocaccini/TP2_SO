#include <stdint.h>
#include "usrlib.h"
#include "apps.h"

/* Estado compartido — todo userland corre en el mismo address space */
static volatile char mvar_shared = 0;
static int           mvar_sem_empty = -1;
static int           mvar_sem_full  = -1;

/* Color unico por lector (consigna: "imprime junto con un identificador
 * unico, por ejemplo un color"). El indice 0 es rojo ("lector rojo"). */
static const uint64_t reader_colors[] = {
    0xFF0000, /* rojo   */
    0x00FF00, /* verde  */
    0x00BFFF, /* celeste*/
    0xFFFF00, /* amarillo*/
    0xFF00FF, /* magenta*/
    0xFF8000  /* naranja*/
};
#define N_COLORS (int)(sizeof(reader_colors) / sizeof(reader_colors[0]))

/* Construye "<prefix><i>" en dst (p. ej. "mvar_w" + 1 -> "mvar_w1"). */
static void build_name(char *dst, const char *prefix, int i) {
    int k = 0;
    while (prefix[k]) { dst[k] = prefix[k]; k++; }
    char num[12];
    intToString(i, num);
    int j = 0;
    while (num[j]) dst[k++] = num[j++];
    dst[k] = '\0';
}

/* Escritores: espera activa LARGA y de BAJA VARIANZA, por lo que son CPU-bound
 * (el cuello de botella). Con prioridades iguales avanzan al mismo ritmo y la
 * salida queda alternada (ABAB); al subirle la prioridad a uno, ese escritor
 * recorre su espera mas rapido (HIGH=3 quantums vs LOW=1) y desbalancea la
 * salida (consigna: mvar 2 1 + nice B high -> ABABABBBABBB...).
 * MVAR_WRITER_BASE controla la velocidad/legibilidad; subilo/bajalo a gusto. */
#define MVAR_WRITER_BASE   3000000u
#define MVAR_WRITER_SPREAD  200000u

/* Lectores: espera corta. Solo consumen lo que producen los escritores, que
 * marcan el ritmo; asi la salida es legible sin afectar la alternancia. */
#define MVAR_READER_SCALE 1u

/* Estado RNG en el stack del hijo: cada proceso avanza su propia secuencia. */
static void busy_wait_writer(uint32_t *rng) {
    *rng = *rng * 1103515245u + 12345u;
    uint32_t iters = MVAR_WRITER_BASE + (*rng >> 13) % MVAR_WRITER_SPREAD;
    volatile uint32_t x = 0;
    for (uint32_t i = 0; i < iters; i++) x++;
}

static void busy_wait_reader(uint32_t *rng) {
    *rng = *rng * 1103515245u + 12345u;
    uint32_t iters = ((*rng >> 16) % 32768u + 8000u) * MVAR_READER_SCALE;
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
        busy_wait_writer(&rng);
        sem_wait(mvar_sem_empty);
        mvar_shared = letra;
        sem_post(mvar_sem_full);
    }
    return 0;
}

static int mvar_reader(char **argv, int argc) {
    if (argc < 2) return 1;
    int ok;
    int idx = satoi(argv[1], &ok);
    if (!ok) return 1;
    uint64_t color = reader_colors[idx % N_COLORS];

    uint32_t rng = (uint32_t)getpid() * 2654435761u + 1u;
    while (1) {
        busy_wait_reader(&rng);
        sem_wait(mvar_sem_full);
        char c = mvar_shared;
        sem_post(mvar_sem_empty);
        drawChar((uint64_t)(unsigned char)c, color); /* imprime con su color */
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
    char name_buf[16];
    char idx_buf[12];

    for (int i = 0; i < W; i++) {
        build_name(name_buf, "mvar_w", i);
        intToString(i, idx_buf);
        char *wargv[] = {name_buf, idx_buf, 0};
        int pid = create_process(mvar_writer, U_MEDIUM, 1, wargv, 2, fds);
        if (pid < 0) {
            printf("mvar: error al crear escritor %d\n", i);
            return 1;
        }
    }

    for (int i = 0; i < R; i++) {
        build_name(name_buf, "mvar_r", i);
        intToString(i, idx_buf);
        char *rargv[] = {name_buf, idx_buf, 0};
        int pid = create_process(mvar_reader, U_MEDIUM, 1, rargv, 2, fds);
        if (pid < 0) {
            printf("mvar: error al crear lector %d\n", i);
            return 1;
        }
    }

    /* El proceso principal termina de inmediato (consigna) */
    return 0;
}
