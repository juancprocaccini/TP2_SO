#include "usrlib.h"

/*
 * test_f7.c — test riguroso de F7:
 *   FD abstraction, syscalls SYS_READ/SYS_WRITE, y stdlib de userland.
 *
 * NO incluye test_util.h para que printf llame a la nuestra (via sys_write),
 * no al alias kprintf que usa drawString directamente.
 */

extern int satoi(const char *str, int *ok);

/* Contador global de errores, compartido entre procesos (mismo binario en 0x400000) */
static volatile int f7_errors = 0;

/* Flag compartido para el sub-test de pipe via FD abstraction */
static volatile int pipe_fd_test_ok = 0;

/* ------------------------------------------------------------------ */
/* Macro de verificación: imprime FAIL y acumula error si falla        */
/* ------------------------------------------------------------------ */
#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("[FAIL] %s\n", (msg)); \
            f7_errors++; \
        } \
    } while (0)

/* ------------------------------------------------------------------ */
/* 1. strlen                                                           */
/* ------------------------------------------------------------------ */
static void test_strlen(void) {
    printf("-- strlen --\n");
    CHECK(strlen("") == 0,           "strlen(\"\") == 0");
    CHECK(strlen("a") == 1,          "strlen(\"a\") == 1");
    CHECK(strlen("hello") == 5,      "strlen(\"hello\") == 5");
    CHECK(strlen("hello world") == 11, "strlen(\"hello world\") == 11");
}

/* ------------------------------------------------------------------ */
/* 2. strcmp                                                           */
/* ------------------------------------------------------------------ */
static void test_strcmp(void) {
    printf("-- strcmp --\n");
    CHECK(strcmp("abc", "abc") == 0, "igual");
    CHECK(strcmp("abc", "abd") < 0,  "menor");
    CHECK(strcmp("abd", "abc") > 0,  "mayor");
    CHECK(strcmp("", "") == 0,       "vacios iguales");
    CHECK(strcmp("", "a") < 0,       "vacio < no-vacio");
    CHECK(strcmp("abc", "ab") > 0,   "prefijo mas largo > corto");
}

/* ------------------------------------------------------------------ */
/* 3. strncasecmp                                                      */
/* ------------------------------------------------------------------ */
static void test_strncasecmp(void) {
    printf("-- strncasecmp --\n");
    CHECK(strncasecmp("Hello", "hello", 5) == 0,   "mayus/minus iguales");
    CHECK(strncasecmp("ABC",   "abc",   3) == 0,   "todo mayus = todo minus");
    CHECK(strncasecmp("abc",   "abd",   3) < 0,    "menos que");
    CHECK(strncasecmp("aBcXX", "AbCYY", 3) == 0,   "primeros 3 iguales, resto no importa");
    CHECK(strncasecmp("abc",   "xyz",   0) == 0,   "n=0 siempre igual");
}

/* ------------------------------------------------------------------ */
/* 4. satoi                                                            */
/* ------------------------------------------------------------------ */
static void test_satoi(void) {
    printf("-- satoi --\n");
    int ok;
    CHECK(satoi("0",    &ok) == 0  && ok,  "0");
    CHECK(satoi("42",   &ok) == 42 && ok,  "42");
    CHECK(satoi("-7",   &ok) == -7 && ok,  "-7");
    CHECK(satoi("1000", &ok) == 1000 && ok, "1000");

    satoi("abc", &ok);
    CHECK(!ok, "abc -> ok=0");

    satoi("12ab", &ok);
    CHECK(!ok, "12ab -> ok=0 (no es numero puro)");

    satoi("", &ok);
    CHECK(!ok, "vacio -> ok=0");
}

/* ------------------------------------------------------------------ */
/* 5. printf — especificadores y valor de retorno                      */
/* ------------------------------------------------------------------ */
static void test_printf_specs(void) {
    printf("-- printf especificadores (inspeccion visual) --\n");
    int n;

    n = printf("  %%d positivo: %d\n", 42);
    CHECK(n > 0, "printf %%d retorna > 0");

    n = printf("  %%d negativo: %d\n", -99);
    CHECK(n > 0, "printf %%d negativo retorna > 0");

    n = printf("  %%d cero: %d\n", 0);
    CHECK(n > 0, "printf %%d cero retorna > 0");

    n = printf("  %%s texto: %s\n", "funciona");
    CHECK(n > 0, "printf %%s retorna > 0");

    n = printf("  %%s null: %s\n", (char *)0);
    CHECK(n > 0, "printf %%s null retorna > 0");

    n = printf("  %%c: %c\n", 'Z');
    CHECK(n > 0, "printf %%c retorna > 0");

    n = printf("  %%x: %x\n", (uint64_t)0xDEADBEEF);
    CHECK(n > 0, "printf %%x retorna > 0");

    n = printf("  %%%%: 100%%\n");
    CHECK(n > 0, "printf %%%% retorna > 0");
}

/* ------------------------------------------------------------------ */
/* 6. fprintf — verifica que fd=1 y fd=2 producen salida              */
/* ------------------------------------------------------------------ */
static void test_fprintf(void) {
    printf("-- fprintf --\n");
    int n;

    n = fprintf(1, "  fprintf(1,stdout): ok\n");
    CHECK(n > 0, "fprintf(1) retorna > 0");

    /* fd=2 es stderr → mismo video, pero ruta independiente */
    n = fprintf(2, "  fprintf(2,stderr): ok\n");
    CHECK(n > 0, "fprintf(2) retorna > 0");
}

/* ------------------------------------------------------------------ */
/* 7. sys_write directo (retorno correcto de bytes)                    */
/* ------------------------------------------------------------------ */
static void test_sys_write_direct(void) {
    printf("-- sys_write directo --\n");

    extern int sys_write(int fd, const char *buf, int n);

    const char *msg = "  sys_write directo: ok\n";
    int len = strlen(msg);
    int written = sys_write(1, msg, len);
    CHECK(written == len, "sys_write retorna n bytes");

    /* Escribir 0 bytes */
    int r = sys_write(1, msg, 0);
    CHECK(r == 0, "sys_write(n=0) retorna 0");
}

/* ------------------------------------------------------------------ */
/* 8. Pipe via FD abstraction                                          */
/*                                                                     */
/* El escritor recibe fds[1]=pipe → su printf() escribe al pipe.      */
/* El lector recibe fds[0]=pipe → su getchar() lee del pipe.          */
/* Ambos abren el pipe en su rol al arrancar (igual que test_pipes).  */
/* ------------------------------------------------------------------ */

static const char *PIPE_MSG = "FD_ABS_OK";

static int pipe_fd_writer(uint64_t argc, char *argv[]) {
    if (argc != 1) return -1;

    int ok;
    int fd_pipe = satoi(argv[0], &ok);
    if (!ok) return -1;

    /* Abrir el pipe como escritor para este proceso */
    if (pipe_open(fd_pipe, WRITER) < 0) return -1;

    /*
     * printf rutea por sys_write(1, ...) → kernel ve fds[1]=fd_pipe
     * y llama pipe_write. Esto es lo que estamos probando.
     */
    printf("%s", PIPE_MSG);

    /* Cerrar el extremo escritor → inyecta EOF al lector */
    pipe_close(fd_pipe);
    return 0;
}

static int pipe_fd_reader(uint64_t argc, char *argv[]) {
    if (argc != 1) return -1;

    int ok;
    int fd_pipe = satoi(argv[0], &ok);
    if (!ok) return -1;

    /* Abrir el pipe como lector para este proceso */
    if (pipe_open(fd_pipe, READER) < 0) return -1;

    /*
     * getchar() → sys_read() → kernel ve fds[0]=fd_pipe
     * y llama pipe_read. Esto es lo que estamos probando.
     */
    char buf[32];
    int i = 0;
    int c;
    while ((c = getchar()) != 0 && i < 31) {
        buf[i++] = (char)c;
    }
    buf[i] = '\0';

    /* Comparar con lo que el escritor envió por printf */
    if (strcmp(buf, PIPE_MSG) == 0) {
        pipe_fd_test_ok = 1;  /* variable compartida con el padre */
    } else {
        /* Usar fprintf(2,...) para que este mensaje salga a video
         * aunque fds[0] esté redirigido al pipe */
        fprintf(2, "  [pipe reader] esperaba '%s', recibi '%s'\n", PIPE_MSG, buf);
    }

    pipe_close(fd_pipe);
    return 0;
}

static void test_pipe_fd_abstraction(void) {
    printf("-- pipe via FD abstraction --\n");

    int fd_pipe = pipe_reserve();
    if (fd_pipe < 0) {
        printf("[FAIL] pipe_reserve fallo\n");
        f7_errors++;
        return;
    }

    /* Convertir fd a string para pasarlo por argv */
    char fd_str[12];
    intToString(fd_pipe, fd_str);
    char *args[] = {fd_str, 0};

    /*
     * Writer: fds[1]=fd_pipe → stdout del proceso va al pipe.
     * Reader: fds[0]=fd_pipe → stdin del proceso viene del pipe.
     * El resto de los fds queda en 2 (stderr → video siempre).
     */
    int writer_fds[3] = {0,       fd_pipe, 2};
    int reader_fds[3] = {fd_pipe, 1,       2};

    pipe_fd_test_ok = 0;

    int pid_r = create_process((int (*)(char **, int))pipe_fd_reader, U_MEDIUM, 1, args, 1, reader_fds);
    int pid_w = create_process((int (*)(char **, int))pipe_fd_writer, U_MEDIUM, 1, args, 1, writer_fds);

    if (pid_r < 0 || pid_w < 0) {
        printf("[FAIL] no se pudieron crear procesos del pipe test\n");
        f7_errors++;
        return;
    }

    waitpid(pid_r, 0);
    waitpid(pid_w, 0);

    CHECK(pipe_fd_test_ok, "reader recibio mensaje correcto via FD abstraction");
}

/* ------------------------------------------------------------------ */
/* Entry point del test                                                */
/* ------------------------------------------------------------------ */
int test_f7(char *argv[], int argc) {
    f7_errors = 0;

    printf("========================================\n");
    printf("      TEST F7: FD abstraction + stdlib  \n");
    printf("========================================\n\n");

    test_strlen();
    test_strcmp();
    test_strncasecmp();
    test_satoi();
    test_printf_specs();
    test_fprintf();
    test_sys_write_direct();
    test_pipe_fd_abstraction();

    printf("\n========================================\n");
    if (f7_errors == 0) {
        printf("[OK] test_f7: todos los checks pasaron\n");
    } else {
        printf("[FAIL] test_f7: %d checks fallaron\n", f7_errors);
    }
    printf("========================================\n\n");

    return f7_errors == 0 ? 0 : -1;
}
