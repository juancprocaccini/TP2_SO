#ifndef PIPE_H
#define PIPE_H

#include "process.h"
#include <stdint.h>

#define MAX_PIPES 64
#define PIPE_BUFFER_SIZE 2048

#define READER 0
#define WRITER 1

typedef struct
{
    pid_t pids[2];        /* [0]=reader, [1]=writer, -1 si está libre */
    int reserved;         /* 1 si fue reservado por pipe_reserve antes de abrirse */
    int closed_by_reader; /* Flag para alertar al escritor si el lector se fue */
    int init_count;       /* Cantidad de extremos abiertos actualmente (0, 1 o 2) */
    uint8_t buf[PIPE_BUFFER_SIZE];
    uint64_t read_pos, write_pos;
    int64_t sem_data;  /* ID del semáforo interno: datos disponibles */
    int64_t sem_space; /* ID del semáforo interno: espacio para dar la vuelta */
} pipe_t;

void pipe_init(void);
int pipe_open(int id, int mode);
int pipe_open_pid(int id, int mode, pid_t pid);
int pipe_open_free(int mode);
int pipe_reserve(void);
int pipe_read(int id, char *buf, int n);
int pipe_write(int id, const char *buf, int n);
int pipe_close(int id, pid_t pid);
pid_t pipe_get_pid(int id, int mode);

#endif