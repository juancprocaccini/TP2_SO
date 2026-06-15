#include <stdint.h>
#include "shell.h"
#include "usrlib.h"
#include "apps.h"

/* tests de la catedra (corren como procesos de usuario, F10).
 * Conservan la convencion de argv de la catedra (argv[0] = primer argumento,
 * argc = cantidad de args sin el nombre del comando). Los adaptadores de abajo
 * descartan el nombre del comando para no modificar el codigo de los tests. */
int test_mm(char **argv, int argc);
int test_processes(char **argv, int argc);
int test_sync(char **argv, int argc);
int test_prio(char **argv, int argc);

static int run_test_mm(char **argv, int argc)        { return test_mm(argv + 1, argc - 1); }
static int run_test_processes(char **argv, int argc) { return test_processes(argv + 1, argc - 1); }
static int run_test_sync(char **argv, int argc)      { return test_sync(argv + 1, argc - 1); }
static int run_test_prio(char **argv, int argc)      { return test_prio(argv + 1, argc - 1); }

#define MAX_ARGS    16
#define LINE_LEN   128

/* --- built-ins (corren en el proceso shell) --- */

static void builtin_help(void) {
    printf("Comandos disponibles:\n");
    printf("  help              - esta ayuda\n");
    printf("  clear             - limpiar pantalla\n");
    printf("  mem               - estado del heap (kernel y usuario)\n");
    printf("  ps                - tabla de procesos\n");
    printf("  loop <ticks>      - bucle de espera activa\n");
    printf("  kill <pid>        - terminar proceso\n");
    printf("  nice <pid> <pri>  - cambiar prioridad (low/medium/high)\n");
    printf("  block <pid>       - bloquear/desbloquear proceso\n");
    printf("  cat               - copia stdin a stdout\n");
    printf("  wc                - cuenta lineas de stdin\n");
    printf("  filter            - filtra vocales de stdin\n");
    printf("  mvar <W> <R>      - W escritores y R lectores sobre variable compartida\n");
    printf("  killall <nombre>  - mata todos los procesos cuyo nombre empieza con <nombre>\n");
    printf("\nTests de la catedra:\n");
    printf("  test_sync, test_prio, test_processes, test_mm\n");
    printf("\nCaracteres especiales:\n");
    printf("  &                 - ejecutar en background\n");
    printf("  |                 - conectar stdout de cmd1 con stdin de cmd2\n");
    printf("  Ctrl+C            - matar proceso en foreground\n");
    printf("  Ctrl+D            - EOF / salir de stdin\n");
}

/* Mata todos los procesos cuyo nombre empieza con 'prefix'. Util para frenar
 * grupos de procesos de fondo (p. ej. 'killall mvar' para todos los workers de
 * mvar) sin tener que matarlos uno por uno. Nunca se mata a si misma. */
static void builtin_killall(const char *prefix) {
    if (prefix == 0 || prefix[0] == '\0') {
        printf("Uso: killall <prefijo-de-nombre>\n");
        return;
    }
    ProcessInfoList *list = 0;
    if (ps(&list) < 0 || list == 0) {
        printf("killall: no se pudo listar procesos\n");
        return;
    }
    int plen = strlen(prefix);
    int self = getpid();
    int killed = 0;
    for (int i = 0; i < list->count; i++) {
        ProcessInfo *e = &list->entries[i];
        if (e->pid == self) continue;
        if (strncasecmp(e->name, prefix, plen) == 0 && kill(e->pid) == 0)
            killed++;
    }
    free_ps(list);
    printf("killall: %d proceso(s) terminado(s)\n", killed);
}

/* --- tabla de comandos como procesos --- */

typedef struct {
    const char *name;
    int (*fn)(char **, int);
} Command;

static const Command cmds[] = {
    { "mem",    cmd_mem    },
    { "ps",     cmd_ps     },
    { "loop",   cmd_loop   },
    { "kill",   cmd_kill   },
    { "nice",   cmd_nice   },
    { "block",  cmd_block  },
    { "cat",    cmd_cat    },
    { "wc",     cmd_wc     },
    { "filter", cmd_filter },
    { "mvar",   cmd_mvar   },
    { "test_mm",        run_test_mm        },
    { "test_processes", run_test_processes },
    { "test_sync",      run_test_sync      },
    { "test_prio",      run_test_prio      },
    { 0, 0 }
};

static const Command *find_cmd(const char *name) {
    for (int i = 0; cmds[i].name; i++) {
        if (strcmp(cmds[i].name, name) == 0)
            return &cmds[i];
    }
    return 0;
}

/* --- parser minimalista --- */

/* Tokeniza en argv in-place (modifica buf). Devuelve argc. */
static int tokenize(char *buf, char **argv, int max_args) {
    int argc = 0;
    char *p = buf;
    while (*p && argc < max_args - 1) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    argv[argc] = 0;
    return argc;
}

/* --- loop principal --- */

void shell_init(void) {
    char line[LINE_LEN];
    char *argv[MAX_ARGS];

    printf("Shell lista. Escribe 'help' para ver los comandos.\n");

    while (1) {
        reap();
        printf("> ");
        gets(line, LINE_LEN);
        if (line[0] == '\0')
            continue;

        int argc = tokenize(line, argv, MAX_ARGS);
        if (argc == 0)
            continue;

        /* Detectar & al final */
        int background = 0;
        if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
            background = 1;
            argv[--argc] = 0;
            if (argc == 0)
                continue;
        }

        /* Detectar | */
        int pipe_pos = -1;
        int pipe_count = 0;
        for (int i = 0; i < argc; i++) {
            if (strcmp(argv[i], "|") == 0) {
                pipe_count++;
                pipe_pos = i;
            }
        }

        if (pipe_count > 1) {
            printf("shell: solo se soporta un pipe por comando\n");
            continue;
        }

        if (pipe_count == 1) {
            int left_argc = pipe_pos;
            char **left_argv = argv;
            argv[pipe_pos] = 0;

            int right_argc = argc - pipe_pos - 1;
            char **right_argv = argv + pipe_pos + 1;

            if (left_argc == 0 || right_argc == 0) {
                printf("shell: sintaxis: uso: cmd1 | cmd2\n");
                continue;
            }

            const char *lcmd = left_argv[0];
            const char *rcmd = right_argv[0];

            if (strcmp(lcmd, "help") == 0 || strcmp(lcmd, "clear") == 0) {
                printf("shell: '%s' es built-in, no puede usarse en un pipe\n", lcmd);
                continue;
            }
            if (strcmp(rcmd, "help") == 0 || strcmp(rcmd, "clear") == 0) {
                printf("shell: '%s' es built-in, no puede usarse en un pipe\n", rcmd);
                continue;
            }

            const Command *lc = find_cmd(lcmd);
            if (!lc) { printf("%s: comando no encontrado\n", lcmd); continue; }
            const Command *rc = find_cmd(rcmd);
            if (!rc) { printf("%s: comando no encontrado\n", rcmd); continue; }

            int fd_pipe = pipe_reserve();
            if (fd_pipe < 0) {
                printf("shell: no se pudo reservar pipe\n");
                continue;
            }

            int writer_fds[3] = { background ? -1 : 0, fd_pipe, 2 };
            int reader_fds[3] = { fd_pipe, 1, 2 };

            int pid1 = create_process(lc->fn, U_MEDIUM, 1, left_argv, left_argc, writer_fds);
            if (pid1 < 0) {
                printf("shell: no se pudo crear proceso escritor\n");
                continue;
            }

            int pid2 = create_process(rc->fn, U_MEDIUM, 1, right_argv, right_argc, reader_fds);
            if (pid2 < 0) {
                kill(pid1);
                printf("shell: no se pudo crear proceso lector\n");
                continue;
            }

            if (background) {
                printf("[bg] pid %d pid %d\n", pid1, pid2);
            } else {
                waitpid(pid2, 0);
                waitpid(pid1, 0);
            }
            continue;
        }

        const char *cmd = argv[0];

        /* built-ins — no soportan background */
        if (strcmp(cmd, "help") == 0) {
            if (background) {
                printf("shell: built-in '%s' no puede correr en background\n", cmd);
                continue;
            }
            builtin_help();
            continue;
        }
        if (strcmp(cmd, "clear") == 0) {
            if (background) {
                printf("shell: built-in '%s' no puede correr en background\n", cmd);
                continue;
            }
            clearScreen();
            continue;
        }
        if (strcmp(cmd, "killall") == 0) {
            if (background) {
                printf("shell: built-in '%s' no puede correr en background\n", cmd);
                continue;
            }
            builtin_killall(argc > 1 ? argv[1] : 0);
            continue;
        }

        /* aplicaciones */
        const Command *c = find_cmd(cmd);
        if (c == 0) {
            printf("%s: comando no encontrado\n", cmd);
            continue;
        }

        int fds[3] = {0, 1, 2};
        if (background)
            fds[0] = -1;

        int pid = create_process(c->fn, U_MEDIUM, 1, argv, argc, fds);
        if (pid < 0) {
            printf("shell: no se pudo crear proceso\n");
            continue;
        }

        if (background) {
            printf("[bg] pid %d\n", pid);
        } else {
            waitpid(pid, 0);
        }
    }
}
