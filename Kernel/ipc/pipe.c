#include "pipe.h"
#include "semaphore.h"
#include "scheduler.h"
#include <stddef.h>

// El arreglo estático de pipes mandatorio del plan
static pipe_t pipes[MAX_PIPES];

void pipe_init(void)
{
    for (int i = 0; i < MAX_PIPES; i++)
    {
        pipes[i].pids[READER] = -1;
        pipes[i].pids[WRITER] = -1;
        pipes[i].reserved = 0;
        pipes[i].closed_by_reader = 0;
        pipes[i].init_count = 0;
        pipes[i].read_pos = 0;
        pipes[i].write_pos = 0;
        // Mapeo determinístico de semáforos kernel requeridos por el plan
        pipes[i].sem_data = SEM_USER_MAX + 2 * i;
        pipes[i].sem_space = SEM_USER_MAX + 2 * i + 1;
    }
}

int pipe_open(int id, int mode)
{
    if (id < 0 || id >= MAX_PIPES || (mode != READER && mode != WRITER))
        return -1;
    return pipe_open_pid(id, mode, scheduler_get_running()->pid);
}

int pipe_open_pid(int id, int mode, pid_t pid)
{
    if (id < 0 || id >= MAX_PIPES || (mode != READER && mode != WRITER))
        return -1;

    pipe_t *pipe = &pipes[id];

    // Si el extremo solicitado ya está ocupado por otro proceso, falla
    if (pipe->pids[mode] != -1)
        return -1;

    pipe->pids[mode] = pid;

    // Si es el primer extremo en abrirse, inicializamos el estado del Pipe
    if (pipe->init_count == 0)
    {
        pipe->closed_by_reader = 0;
        pipe->read_pos = 0;
        pipe->write_pos = 0;
        pipe->reserved = 0;

        // Abrimos los semáforos en el rango exclusivo de Kernel
        ksem_open_kernel_side(pipe->sem_data, 0);  // 0 bytes listos para leer al inicio
        ksem_open_kernel_side(pipe->sem_space, 1); // 1 vuelta libre garantizada para escribir
    }
    pipe->init_count++;
    return id;
}

int pipe_open_free(int mode)
{
    for (int i = 0; i < MAX_PIPES; i++)
    {
        if (pipes[i].pids[READER] == -1 && pipes[i].pids[WRITER] == -1 && !pipes[i].reserved)
        {
            if (pipe_open(i, mode) == i)
            {
                return i;
            }
        }
    }
    return -1;
}

int pipe_reserve(void)
{
    for (int i = 0; i < MAX_PIPES; i++)
    {
        if (pipes[i].pids[READER] == -1 && pipes[i].pids[WRITER] == -1 && !pipes[i].reserved)
        {
            pipes[i].reserved = 1;
            return i;
        }
    }
    return -1;
}

int pipe_read(int id, char *buf, int n)
{
    if (id < 0 || id >= MAX_PIPES || n <= 0)
        return -1;

    pipe_t *pipe = &pipes[id];
    int bytes_read = 0;

    while (bytes_read < n)
    {
        // Se bloquea si no hay bytes disponibles puestos por el escritor
        if (ksem_wait(pipe->sem_data) < 0)
            return -1;

        char c = pipe->buf[pipe->read_pos];
        pipe->read_pos++;

        // Si es el byte EOF (0), terminamos la lectura de forma limpia (retornando lo leído hasta el momento)
        if (c == 0)
        {
            if (pipe->read_pos == PIPE_BUFFER_SIZE)
            {
                pipe->read_pos = 0;
                ksem_post(pipe->sem_space);
            }
            break;
        }

        buf[bytes_read++] = c;

        // Si el lector completó una vuelta entera al búfer circular, resetea posición y avisa al escritor
        if (pipe->read_pos == PIPE_BUFFER_SIZE)
        {
            pipe->read_pos = 0;
            ksem_post(pipe->sem_space);
        }
    }
    return bytes_read;
}

int pipe_write(int id, const char *buf, int n)
{
    if (id < 0 || id >= MAX_PIPES || n < 0)
        return -1;

    pipe_t *pipe = &pipes[id];
    if (pipe->closed_by_reader)
        return -1;

    int bytes_written = 0;

    while (bytes_written < n)
    {
        if (pipe->closed_by_reader)
        {
            return bytes_written > 0 ? bytes_written : -1;
        }

        pipe->buf[pipe->write_pos] = buf[bytes_written];
        pipe->write_pos++;
        bytes_written++;

        // Avisamos al lector que hay un nuevo byte disponible
        ksem_post(pipe->sem_data);

        // Si el escritor llenó la vuelta actual del búfer circular, se bloquea esperando espacio libre
        if (pipe->write_pos == PIPE_BUFFER_SIZE)
        {
            if (ksem_wait(pipe->sem_space) < 0)
                return -1;
            pipe->write_pos = 0;
        }
    }
    return bytes_written;
}

int pipe_close(int id, pid_t pid)
{
    if (id < 0 || id >= MAX_PIPES)
        return -1;

    pipe_t *pipe = &pipes[id];
    int found = 0;

    if (pipe->pids[READER] == pid)
    {
        pipe->pids[READER] = -1;
        pipe->closed_by_reader = 1;
        found = 1;
        // Regla del plan: Destrabamos inmediatamente al escritor bloqueado usando no_yield para evitar deadlocks
        ksem_post_no_yield(pipe->sem_space);
    }

    if (pipe->pids[WRITER] == pid)
    {
        pipe->pids[WRITER] = -1;
        found = 1;
        // Regla del plan: El cierre del escritor inyecta un byte explícito de EOF (0) si el lector sigue ahí
        if (!pipe->closed_by_reader)
        {
            pipe->buf[pipe->write_pos] = 0;
            pipe->write_pos++;
            ksem_post(pipe->sem_data);
            if (pipe->write_pos == PIPE_BUFFER_SIZE)
            {
                pipe->write_pos = 0;
            }
        }
    }

    if (!found)
        return -1;

    // Si ambos extremos se cerraron por completo, liberamos los recursos y semáforos del pipe
    if (pipe->pids[READER] == -1 && pipe->pids[WRITER] == -1)
    {
        ksem_close(pipe->sem_data);
        ksem_close(pipe->sem_space);
        pipe->reserved = 0;
        pipe->closed_by_reader = 0;
        pipe->init_count = 0;
        pipe->read_pos = 0;
        pipe->write_pos = 0;
    }

    return 0;
}

int pipe_close_quiet(int id, pid_t pid)
{
    if (id < 0 || id >= MAX_PIPES)
        return -1;

    pipe_t *pipe = &pipes[id];
    int found = 0;

    if (pipe->pids[READER] == pid)
    {
        pipe->pids[READER] = -1;
        pipe->closed_by_reader = 1;
        found = 1;
        ksem_post_no_yield(pipe->sem_space);
    }

    if (pipe->pids[WRITER] == pid)
    {
        pipe->pids[WRITER] = -1;
        found = 1;
        if (!pipe->closed_by_reader)
        {
            /* write_pos puede ser PIPE_BUFFER_SIZE si el writer murió bloqueado
             * en ksem_wait(sem_space): proteger el acceso al buffer. */
            if (pipe->write_pos >= PIPE_BUFFER_SIZE)
                pipe->write_pos = 0;
            pipe->buf[pipe->write_pos] = 0;
            pipe->write_pos++;
            ksem_post_no_yield(pipe->sem_data);
            if (pipe->write_pos == PIPE_BUFFER_SIZE)
                pipe->write_pos = 0;
        }
    }

    if (!found)
        return -1;

    if (pipe->pids[READER] == -1 && pipe->pids[WRITER] == -1)
    {
        ksem_close(pipe->sem_data);
        ksem_close(pipe->sem_space);
        pipe->reserved = 0;
        pipe->closed_by_reader = 0;
        pipe->init_count = 0;
        pipe->read_pos = 0;
        pipe->write_pos = 0;
    }

    return 0;
}

pid_t pipe_get_pid(int id, int mode)
{
    if (id < 0 || id >= MAX_PIPES || (mode != READER && mode != WRITER))
        return -1;
    return pipes[id].pids[mode];
}