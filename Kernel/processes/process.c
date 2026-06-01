#include <process.h>
#include <scheduler.h>
#include <mem.h>
#include <lib.h>
#include <defs.h>
#include <stddef.h>

PCB pcbs[MAX_PROCESSES];

extern void _cli(void);
extern void timer_tick(void);

static void make_zombie(pid_t pid, int status) {
    pcbs[pid].exit_status = status;
    pcbs[pid].state = ZOMBIE;
    /*
     * TODO(F4): al pasar a ZOMBIE hay que llamar a scheduler_unschedule(pcb)
     * para sacarlo de ready_list. Sin eso, el scheduler vuelve a planificar
     * este wrapper, que reentra al while(1) y quema CPU hasta que el padre
     * lo reape. Se deja acá porque la limpieza completa (waiters, fds, argv,
     * stack) es responsabilidad de la F4 y se diseña en conjunto.
     */
}

/*
 * Copia el nombre al PCB respetando el bound de 32 bytes (incluido '\0').
 * Sin <string.h> propio para strncpy, se hace a mano.
 */
static void set_process_name(PCB *pcb, const char *src) {
    int i = 0;
    if (src) {
        while (i < (int)sizeof(pcb->name) - 1 && src[i] != '\0') {
            pcb->name[i] = src[i];
            i++;
        }
    }
    pcb->name[i] = '\0';
}

/*
 * Libera lo allocado para argv en caso de error parcial en process_create.
 * Acepta NULLs internos (mem_alloc puede haber fallado en medio del loop).
 */
static void free_argv_copy(char **argv_copy, int count) {
    if (!argv_copy) return;
    for (int i = 0; i < count; i++) {
        if (argv_copy[i]) mem_free(argv_copy[i]);
    }
    mem_free(argv_copy);
}

void process_wrapper(entry_t rip, char **argv, int argc, pid_t pid) {
    int ret = rip(argv, argc);
    _cli();
    make_zombie(pid, ret);
    timer_tick();
    while (1);
}

pid_t process_create(entry_t rip, priority_t pri, int killable, char **argv, int argc, int fds[3]) {
    pid_t pid = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pcbs[i].state == FREE) {
            pid = i;
            break;
        }
    }

    if (pid == -1) return -1;

    void *stack = mem_alloc(STACK_SIZE);
    if (!stack) return -1;

    char **new_argv = NULL;
    if (argc > 0 && argv) {
        new_argv = (char **)mem_alloc((argc + 1) * sizeof(char *));
        if (!new_argv) {
            mem_free(stack);
            return -1;
        }
        for (int i = 0; i <= argc; i++) new_argv[i] = NULL;
        for (int i = 0; i < argc; i++) {
            int len = strlen(argv[i]);
            new_argv[i] = (char *)mem_alloc(len + 1);
            if (!new_argv[i]) {
                free_argv_copy(new_argv, argc);
                mem_free(stack);
                return -1;
            }
            strcpy(new_argv[i], argv[i]);
        }
    }

    PCB *pcb = &pcbs[pid];
    pcb->pid = pid;
    pcb->priority = pri;
    pcb->stack_base = (uint64_t)stack;
    pcb->argc = argc;
    pcb->argv = new_argv;
    pcb->killable = killable;
    pcb->waiting_me = NULL;
    pcb->waiting_for = NULL;
    pcb->blocked_by_sem = -1;
    pcb->exit_status = 0;

    /*
     * Nombre: usamos argv[0] si está, sino cadena vacía. La consigna pide
     * que `ps` muestre un nombre por proceso.
     */
    set_process_name(pcb, (argc > 0 && new_argv) ? new_argv[0] : NULL);

    if (fds) {
        pcb->fds[0] = fds[0];
        pcb->fds[1] = fds[1];
        pcb->fds[2] = fds[2];
    } else {
        pcb->fds[0] = 0;
        pcb->fds[1] = 1;
        pcb->fds[2] = 2;
    }

    /*
     * Forjado de Stack.
     * Simula el marco de una interrupción para que iretq restaure el contexto.
     * El orden de empuje desciende en memoria y debe encajar con popState (RAX -> R15).
     *
     * Alineación: el payload de mem_alloc es ALIGN8, no ALIGN16; sumarle STACK_SIZE
     * deja stack_top con mod 16 variable. La ABI SysV-AMD64 espera RSP ≡ 8 (mod 16)
     * al entrar a process_wrapper (estado equivalente a "justo después de un CALL").
     * Si llega en 0, instrucciones SSE con operando alineado a 16 pueden lanzar #GP.
     * Bajamos stack_top al primer múltiplo de 16 igual o menor y le restamos 8.
     */
    uint64_t stack_top = ((uint64_t)stack + STACK_SIZE) & ~((uint64_t)0xF);
    stack_top -= 8;
    uint64_t *stack_frame = (uint64_t *)stack_top;

    *(--stack_frame) = 0x00;                        /* SS */
    *(--stack_frame) = stack_top;                   /* RSP original */
    *(--stack_frame) = 0x202;                       /* RFLAGS (IF=1) */
    *(--stack_frame) = 0x08;                        /* CS */
    *(--stack_frame) = (uint64_t)process_wrapper;   /* RIP */
    
    *(--stack_frame) = 0;                           /* RAX */
    *(--stack_frame) = 0;                           /* RBX */
    *(--stack_frame) = (uint64_t)pid;               /* RCX */
    *(--stack_frame) = (uint64_t)argc;              /* RDX */
    *(--stack_frame) = (uint64_t)new_argv;          /* RSI */
    *(--stack_frame) = (uint64_t)rip;               /* RDI */
    *(--stack_frame) = 0;                           /* RBP */
    *(--stack_frame) = 0;                           /* R8 */
    *(--stack_frame) = 0;                           /* R9 */
    *(--stack_frame) = 0;                           /* R10 */
    *(--stack_frame) = 0;                           /* R11 */
    *(--stack_frame) = 0;                           /* R12 */
    *(--stack_frame) = 0;                           /* R13 */
    *(--stack_frame) = 0;                           /* R14 */
    *(--stack_frame) = 0;                           /* R15 */

    pcb->rsp = (uint64_t)stack_frame;

    /*
     * Encolar al final, una vez que el PCB está consistente. scheduler_ready
     * marca READY y agrega a ready_list; si list_add falla, deshacemos todo
     * para no dejar un PCB ocupado que el scheduler nunca va a planificar.
     * Hasta este punto pcb->state seguía siendo FREE (lo que mantiene el slot
     * libre de cara a una recuperación) — recién acá lo damos por vivo.
     */
    if (scheduler_ready(pcb) < 0) {
        free_argv_copy(new_argv, argc);
        mem_free(stack);
        pcb->state = FREE;
        return -1;
    }

    return pid;
}

PCB *process_get(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) {
        return NULL;
    }
    return &pcbs[pid];
}