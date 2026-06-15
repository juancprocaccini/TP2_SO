#include "usrlib.h"
#include "test_util.h"

static int str_length(const char *str)
{
    int len = 0;
    while (str[len] != '\0')
        len++;
    return len;
}

// ---------------------------------------------------------
// PROCESO ESCRITOR (Corregido el orden de parámetros)
// ---------------------------------------------------------
int pipe_writer_worker(uint64_t argc, char *argv[])
{
    if (argc != 1)
        return -1;
    int fd = satoiOld(argv[0]);

    if (pipe_open(fd, WRITER) < 0)
    {
        printf("Writer: Error abriendo pipe %d\n", fd);
        return -1;
    }

    char *msg1 = "Hola desde el escritor! ";
    char *msg2 = "Aca va otra parte. ";
    char *msg3 = "Y este es el final.\n";

    printf("Writer: Escribiendo parte 1...\n");
    pipe_write(fd, msg1, str_length(msg1));
    yield();

    printf("Writer: Escribiendo parte 2...\n");
    pipe_write(fd, msg2, str_length(msg2));
    yield();

    printf("Writer: Escribiendo parte 3...\n");
    pipe_write(fd, msg3, str_length(msg3));

    printf("Writer: Cerrando pipe (enviando EOF)...\n");
    pipe_close(fd);

    return 0;
}

// ---------------------------------------------------------
// PROCESO LECTOR (Corregido el orden de parámetros)
// ---------------------------------------------------------
int pipe_reader_worker(uint64_t argc, char *argv[])
{
    if (argc != 1)
        return -1;
    int fd = satoiOld(argv[0]);

    if (pipe_open(fd, READER) < 0)
    {
        printf("Reader: Error abriendo pipe %d\n", fd);
        return -1;
    }

    char buffer[64];
    int bytes_read;
    int total_read = 0;

    printf("Reader: Esperando datos...\n");

    while ((bytes_read = pipe_read(fd, buffer, 63)) > 0)
    {
        buffer[bytes_read] = '\0';
        printf("Reader leyo (%d bytes): %s\n", bytes_read, buffer);
        total_read += bytes_read;
    }

    printf("Reader: EOF detectado. Total leido: %d bytes.\n", total_read);
    pipe_close(fd);
    return 0;
}

// ---------------------------------------------------------
// ORQUESTADOR DEL TEST
// ---------------------------------------------------------
int test_pipes(char *argv[], int argc)
{
    printf("=== INICIANDO TEST DE PIPES ===\n");

    
    int fd = pipe_reserve();
    if (fd < 0)
    {
        printf("test_pipes: Error al reservar pipe.\n");
        return -1;
    }

    char fd_str[10];
    intToString(fd, fd_str);

    // Dejar el arreglo terminado en 0 (NULL) evita problemas de lectura en Kernel
    char *args[] = {fd_str, 0};
    int fds[3] = {0, 1, 2};

    printf("test_pipes: Pipe reservado con FD %d\n", fd);

    // Casteamos temporalmente a la firma que espera tu wrapper de usrlib
    int pid_reader = create_process((int (*)(char **, int))pipe_reader_worker, 1, 1, args, 1, fds);
    int pid_writer = create_process((int (*)(char **, int))pipe_writer_worker, 1, 1, args, 1, fds);

    // Validación estricta para no dar falsos positivos si falla el create
    if (pid_reader < 0 || pid_writer < 0)
    {
        printf("test_pipes: ERROR al crear los procesos hijos.\n");
        return -1;
    }

    waitpid(pid_reader, 0);
    waitpid(pid_writer, 0);

    return 0;
}