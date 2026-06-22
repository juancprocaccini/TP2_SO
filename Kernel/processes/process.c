#include <process.h>
#include <scheduler.h>
#include <mem.h>
#include <lib.h>
#include <defs.h>
#include <stddef.h>
#include <semaphore.h>
#include <pipe.h>
#include <keyboardDriver.h>

PCB pcbs[MAX_PROCESSES];

extern void _cli(void);
extern void timer_tick(void);
extern void _sti(void);

extern PCB *scheduler_get_running(void);
extern void scheduler_unschedule(PCB *p);
extern void scheduler_block(PCB *p);
extern int is_foreground(pid_t pid);

static void close_process_pipes(PCB *p) {
    for (int i = 0; i < 3; i++)
        if (p->fds[i] >= 3)
            pipe_close_quiet(p->fds[i] - 3, p->pid);
}

void ctrlc_handler(void) {
    PCB *shell = scheduler_get_shell();
    if (shell == NULL) return;
    PCB *fg = shell->waiting_for;
    if (fg == NULL || fg == shell || fg->state == FREE) return;
    pid_t fg_pid = fg->pid;
    /* Vaciar el buffer de teclado antes de matar: lo que se tipeó y el foreground
     * no alcanzó a consumir no debe filtrarse al próximo comando de la shell.
     * Se hace antes del kill porque el process_kill del foreground puede ser un
     * self-kill que no retorna. */
    kbd_clear_buffer();
    int rfd = fg->fds[0];
    if (rfd >= 3) {
        pid_t w = pipe_get_pid(rfd - 3, WRITER);
        if (w >= 0 && w != fg_pid) {
            /* Matar fg primero garantiza que la shell siempre se despierte:
             * si un timer preempta process_kill(writer) en el _sti() interno,
             * la shell corre, re-bloquea en waitpid(writer) y setea
             * writer->waiting_me = shell; cuando process_kill(writer) retoma,
             * la despierta de nuevo. Matar writer primero invierte el orden y
             * puede dejar la shell re-bloqueada esperando un proceso a medio
             * matar cuyo waiting_me ya fue verificado (era NULL) → deadlock. */
            process_kill(fg_pid);
            process_kill(w);
            return;
        }
    }
    process_kill(fg_pid);
}

static void make_zombie(pid_t pid, int status) {
    close_process_pipes(&pcbs[pid]);
    /* pipe_close_quiet hace _sti() internamente; restaurar atomicidad antes
     * de modificar estado del PCB y las listas del scheduler. */
    _cli();
    pcbs[pid].exit_status = status;
    pcbs[pid].state = ZOMBIE;
    scheduler_unschedule(&pcbs[pid]);
    if (pcbs[pid].waiting_me != NULL)
    {
        pcbs[pid].waiting_me->state = READY;
        scheduler_ready(pcbs[pid].waiting_me);
    }
}

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

void process_wrapper(entry_t rip, char **argv, int argc, pid_t pid)
{
    int ret;
    // "D" asigna argc directo a RDI, "S" asigna argv a RSI, "=a" recupera EAX (retorno)
    __asm__ volatile(
        "call *%3"
        : "=a"(ret)
        : "D"((uint64_t)argv), "S"((uint64_t)argc), "r"((uint64_t)rip)
        : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");

    _cli();
    make_zombie(pid, ret);
    timer_tick();

    while (1)
        ;
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

    /* Registrar pipes DESPUÉS de scheduler_ready: ksem_open_kernel_side hace
     * _sti() y si se pusiera antes expone mem_alloc de list_add con IF=1.
     * En uniprocessor el proceso nuevo no toma CPU hasta que el syscall retorne,
     * así que el pipe queda consistente antes de que el hijo arranque. */
    if (fds && fds[0] >= 3)
        pipe_open_pid(fds[0] - 3, READER, pid);
    if (fds && fds[1] >= 3)
        pipe_open_pid(fds[1] - 3, WRITER, pid);

    return pid;
}

PCB *process_get(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) {
        return NULL;
    }
    return &pcbs[pid];
}

void process_exit(int status) {
    _cli();
    PCB *current = scheduler_get_running();
    make_zombie(current->pid, status);

    // Forzamos el cambio de contexto
    timer_tick();
    while (1);
}

pid_t process_getpid(void) {
    return scheduler_get_running()->pid;
}

void process_yield(void) {
    scheduler_yield();
}

int process_nice(pid_t pid, priority_t priority) {
    if (pid < 0 || pid >= MAX_PROCESSES || priority < LOW || priority > HIGH)
        return -1;
    PCB *p = &pcbs[pid];
    if (p->state == FREE)
        return -1;

    p->priority = priority;
    return 0;
}

int process_block(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;
    PCB *p = &pcbs[pid];
    if (p->state == FREE || p->state == ZOMBIE || p->state == BLOCKED)
        return -1;

    _cli();
    scheduler_block(p);
    _sti();
    return 0;
}

int process_set_block_by_sem(pid_t pid,int id) {
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;
    PCB *p = &pcbs[pid];
    p->blocked_by_sem = id;
    return 0;
}

int process_unblock(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;
    PCB *p = &pcbs[pid];
    if (p->state != BLOCKED)
        return -1;

    _cli();
    scheduler_ready(p);
    _sti();
    return 0;
}

int process_get_status(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES || pcbs[pid].state == FREE)
        return -1;
    return pcbs[pid].state;
}

void process_get_my_fds(int fds_out[3]) {
    PCB *current = scheduler_get_running();
    fds_out[0] = current->fds[0];
    fds_out[1] = current->fds[1];
    fds_out[2] = current->fds[2];
}

void process_reap_orphans(void) {
    _cli();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &pcbs[i];
        if (p->state != ZOMBIE || p->waiting_me != NULL)
            continue;
        if (p->stack_base) {
            mem_free((void *)p->stack_base);
            p->stack_base = 0;
        }
        if (p->argv) {
            free_argv_copy(p->argv, p->argc);
            p->argv = NULL;
        }
        p->state = FREE;
    }
    _sti();
}

int process_kill(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;
    PCB *p = &pcbs[pid];
    if (p->state == FREE || !p->killable)
        return -1;
    if (p == scheduler_get_shell())
        return -1;

    _cli(); // Operación atómica de Kernel

    scheduler_unschedule(p);

    if (p->waiting_me != NULL)
    {
        scheduler_ready(p->waiting_me);
    }
    if (p->waiting_for != NULL)
    {
        p->waiting_for->waiting_me = NULL;
    }
    
    kbd_clear_waiter(p);

    // Desalojo seguro de las colas de Semáforos (F5 ready)
    if (p->blocked_by_sem != -1)
    {
        ksem_remove_waiter(p->blocked_by_sem, p);
    }

    /* pipe_close_quiet llama ksem_post_no_yield que hace _sti() internamente;
     * segundo _cli() después para retomar atomicidad en el teardown final. */
    close_process_pipes(p);
    _cli();

    // Liberación completa de recursos
    if (p->stack_base)
    {
        mem_free((void *)p->stack_base);
        p->stack_base = 0;
    }
    if (p->argv)
    {
        free_argv_copy(p->argv, p->argc);
        p->argv = NULL;
    }

    p->state = FREE;

    // Si el proceso se suicidó a sí mismo, hacemos context-switch inmediato
    if (p == scheduler_get_running())
    {
        timer_tick();
        while (1)
            ;
    }

    return 0;
}

int process_waitpid(pid_t pid, int *ret_out)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;
    PCB *target = &pcbs[pid];
    PCB *me = scheduler_get_running();

    if (target->state == FREE || target->waiting_me != NULL || target == me)
    {
        return -1;
    }

    // Si todavía está vivo, me bloqueo esperando a que termine
    if (target->state != ZOMBIE)
    {
        _cli();
        target->waiting_me = me;
        me->waiting_for = target;
        me->state = BLOCKED;
        scheduler_block(me);
        
        // Al despertar, limpiamos la relación
        _sti();
        me->waiting_for = NULL;
    }

    // Si el proceso objetivo fue asesinado por un kill en el medio del wait, salimos con error
    if (target->state == FREE || target->state != ZOMBIE)
    {
        return -1;
    }

    // Cosechamos de manera segura (Reap)
    if (ret_out != NULL)
    {
        *ret_out = (int)target->exit_status;
    }

    // Liberamos definitivamente la memoria que retuvo el Zombie
    if (target->stack_base)
    {
        mem_free((void *)target->stack_base);
        target->stack_base = 0;
    }
    if (target->argv)
    {
        free_argv_copy(target->argv, target->argc);
        target->argv = NULL;
    }

    target->state = FREE;
    return pid;
}

int process_ps(ProcessInfoList **info_list_out){
    if (!info_list_out)
        return -1;

    _cli();

    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (pcbs[i].state != FREE)
            count++;
    }

    // Alocamos la lista contenedora en el Kernel Heap
    ProcessInfoList *list = (ProcessInfoList *)mem_alloc(sizeof(ProcessInfoList));
    if (!list)
    {
        _sti();
        return -1;
    }

    list->count = count;
    list->entries = NULL;

    if (count > 0)
    {
        list->entries = (ProcessInfo *)mem_alloc(count * sizeof(ProcessInfo));
        if (!list->entries)
        {
            mem_free(list);
            _sti();
            return -1;
        }

        int idx = 0;
        for (int i = 0; i < MAX_PROCESSES; i++)
        {
            if (pcbs[i].state != FREE)
            {
                ProcessInfo *entry = &list->entries[idx];
                entry->pid = pcbs[i].pid;

                int j = 0;
                while (pcbs[i].name[j] != '\0' && j < 31)
                {
                    entry->name[j] = pcbs[i].name[j];
                    j++;
                }
                entry->name[j] = '\0';

                entry->priority = pcbs[i].priority;
                entry->state = pcbs[i].state;
                entry->rsp = pcbs[i].rsp;
                entry->stack_base = pcbs[i].stack_base;
                entry->foreground = is_foreground(pcbs[i].pid);
                entry->fds[0] = pcbs[i].fds[0];
                entry->fds[1] = pcbs[i].fds[1];
                entry->fds[2] = pcbs[i].fds[2];
                idx++;
            }
        }
    }

    _sti();

    *info_list_out = list;
    return count;
}

void process_free_ps(ProcessInfoList *list)
{
    if (list)
    {
        if (list->entries)
            mem_free(list->entries);
        mem_free(list);
    }
}