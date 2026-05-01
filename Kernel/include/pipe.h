#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

/*
 * Interfaz de pipes unidireccionales bloqueantes.
 * Implementaciones en Kernel/ipc/pipe.c.
 *
 * pipe_open: crea un pipe anónimo, deja en fds[0] el extremo de lectura
 *            y en fds[1] el extremo de escritura.
 * pipe_open_named: igual pero buscable por nombre (si existe, abre el
 *            mismo pipe; si no, lo crea).
 */

int pipe_open(int fds[2]);
int pipe_open_named(const char *name, int fds[2]);
int pipe_read(int fd, char *buf, uint64_t count);
int pipe_write(int fd, const char *buf, uint64_t count);
int pipe_close(int fd);

#endif
