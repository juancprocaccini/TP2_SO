#include <stdint.h>
#include "shell.h"
#include "usrlib.h"
#include "apps.h"

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
    printf("\nTests de la catedra:\n");
    printf("  test_sync, test_prio, test_processes, test_mm\n");
    printf("\nCaracteres especiales:\n");
    printf("  &                 - ejecutar en background\n");
    printf("  Ctrl+C            - matar proceso en foreground\n");
    printf("  Ctrl+D            - EOF / salir de stdin\n");
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
