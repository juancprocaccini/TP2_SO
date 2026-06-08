/*
 * process.c — gestión de procesos del kernel.
 *
 * F2: creación de procesos, forjado de stack, process_wrapper.
 * F4: ciclo de vida completo (exit, kill, wait, nice, block/unblock, ps).
 */

#include <process.h>
#include <scheduler.h>
#include <mem.h>
#include <lib.h>
#include <defs.h>
#include <stddef.h>

/* Array estático de PCBs; PID == índice. */
PCB pcbs[MAX_PROCESSES];

extern void _cli(void);
extern void timer_tick(void);

/* ============================================================
 * Helpers privados
 * ============================================================ */

/*
 * Copia el nombre al PCB con bound de 32 bytes (incluido '\0').
 */
static void set_process_name(PCB *pcb, const char *src)
{
    int i = 0;
    if (src)
    {
        while (i < (int)sizeof(pcb->name) - 1 && src[i] != '\0')
        {
            pcb->name[i] = src[i];
            i++;
        }
    }
    pcb->name[i] = '\0';
}

/*
 * Libera el vector argv copiado en el kernel heap.
 * Tolera NULLs internos: mem_alloc puede haber fallado a mitad de camino.
 */
static void free_argv_copy(char **argv_copy, int count)
{
    if (!argv_copy)
        return;
    for (int i = 0; i < count; i++)
    {
        if (argv_copy[i])
            mem_free(argv_copy[i]);
    }
    mem_free(argv_copy);
}

/*
 * Libera stack y argv de un PCB. Usada tanto por process_exit (a través de
 * waitpid) como por process_kill cuando no hay waiter.
 * Deja stack_base=0 y argv=NULL para que una doble liberación sea inofensiva.
 */
static void free_process_resources(PCB *p)
{
    if (p->argv)
    {
        free_argv_copy(p->argv, p->argc);
        p->argv = NULL;
    }
    if (p->stack_base)
    {
        mem_free((void *)p->stack_base);
        p->stack_base = 0;
    }
}

/* ============================================================
 * F2 — Infraestructura de creación
 * ============================================================ */

/*
 * Punto de entrada de todo proceso. Llama a la función real y, al retornar,
 * termina el proceso limpiamente vía process_exit (que no retorna).
 *
 * Los argumentos llegan por registros gracias al stack forjado en
 * process_create: RDI=rip, RSI=argv, RDX=argc, RCX=pid.
 */
void process_wrapper(entry_t rip, char **argv, int argc, pid_t pid)
{
    int ret = rip(argv, argc);
    _cli();
    /*
     * process_exit desplanifica el proceso, notifica al waiter y fuerza
     * context switch. No retorna; el while(1) es por el compilador.
     */
    process_exit(ret);
    (void)pid; /* pid se usó para forjar el stack; acá ya no hace falta */
    while (1)
        ;
}

pid_t process_create(entry_t rip, priority_t pri, int killable,
                     char **argv, int argc, int fds[3])
{
    /* Buscar slot libre; PID == índice. */
    pid_t pid = -1;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (pcbs[i].state == FREE)
        {
            pid = i;
            break;
        }
    }
    if (pid == -1)
        return -1;

    void *stack = mem_alloc(STACK_SIZE);
    if (!stack)
        return -1;

    /* Copiar argv al kernel heap para que el hijo tenga sus propios strings. */
    char **new_argv = NULL;
    if (argc > 0 && argv)
    {
        new_argv = (char **)mem_alloc((argc + 1) * sizeof(char *));
        if (!new_argv)
        {
            mem_free(stack);
            return -1;
        }

        for (int i = 0; i <= argc; i++)
            new_argv[i] = NULL;
        for (int i = 0; i < argc; i++)
        {
            int len = strlen(argv[i]);
            new_argv[i] = (char *)mem_alloc(len + 1);
            if (!new_argv[i])
            {
                free_argv_copy(new_argv, argc);
                mem_free(stack);
                return -1;
            }
            strcpy(new_argv[i], argv[i]);
        }
    }

    /* Inicializar PCB. */
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

    set_process_name(pcb, (argc > 0 && new_argv) ? new_argv[0] : NULL);

    if (fds)
    {
        pcb->fds[0] = fds[0];
        pcb->fds[1] = fds[1];
        pcb->fds[2] = fds[2];
    }
    else
    {
        /* Herencia de terminal por defecto. */
        pcb->fds[0] = 0;
        pcb->fds[1] = 1;
        pcb->fds[2] = 2;
    }

    /*
     * Forjado de stack.
     *
     * Simula el marco que deja una interrupción para que iretq en
     * _irq00Handler restaure el contexto correctamente.
     * popState desapila: R15→R8, RBP, RDI, RSI, RDX, RCX, RBX, RAX.
     * Encima quedan SS, RSP, RFLAGS, CS, RIP para el iretq.
     *
     * Alineación: ABI SysV-AMD64 exige RSP ≡ 8 (mod 16) al entrar a una
     * función (equivalente a "justo después de un CALL"). Bajamos al primer
     * múltiplo de 16 y restamos 8.
     */
    uint64_t stack_top = ((uint64_t)stack + STACK_SIZE) & ~((uint64_t)0xF);
    stack_top -= 8;
    uint64_t *sf = (uint64_t *)stack_top;

    /* Marco de interrupción (iretq lo consume en orden inverso al push): */
    *(--sf) = 0x00;                      /* SS */
    *(--sf) = stack_top;                 /* RSP */
    *(--sf) = 0x202;                     /* RFLAGS: IF=1 */
    *(--sf) = 0x08;                      /* CS (kernel code segment) */
    *(--sf) = (uint64_t)process_wrapper; /* RIP */

    /* Registros de propósito general (popState order): */
    *(--sf) = 0;                  /* RAX */
    *(--sf) = 0;                  /* RBX */
    *(--sf) = (uint64_t)pid;      /* RCX → 4.º arg de process_wrapper */
    *(--sf) = (uint64_t)argc;     /* RDX → 3.º arg */
    *(--sf) = (uint64_t)new_argv; /* RSI → 2.º arg */
    *(--sf) = (uint64_t)rip;      /* RDI → 1.º arg */
    *(--sf) = 0;                  /* RBP */
    *(--sf) = 0;                  /* R8  */
    *(--sf) = 0;                  /* R9  */
    *(--sf) = 0;                  /* R10 */
    *(--sf) = 0;                  /* R11 */
    *(--sf) = 0;                  /* R12 */
    *(--sf) = 0;                  /* R13 */
    *(--sf) = 0;                  /* R14 */
    *(--sf) = 0;                  /* R15 */

    pcb->rsp = (uint64_t)sf;

    /*
     * Encolar al final: PCB está consistente, scheduler_ready lo marca READY
     * y lo agrega a ready_list. Si falla, deshacemos para no dejar un slot
     * ocupado que el scheduler nunca planifique.
     */
    if (scheduler_ready(pcb) < 0)
    {
        free_argv_copy(new_argv, argc);
        mem_free(stack);
        pcb->state = FREE;
        return -1;
    }

    return pid;
}

PCB *process_get(pid_t pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return NULL;
    return &pcbs[pid];
}

/* ============================================================
 * F4 — Ciclo de vida
 * ============================================================ */

/*
 * Termina el proceso running con el código dado.
 *
 * Secuencia:
 *   1. Desplanificar (sacar de ready/blocked list).
 *   2. Guardar exit_status y marcar ZOMBIE.
 *   3. Si hay un waiter bloqueado, despertarlo.
 *   4. No liberamos recursos acá: el padre los libera en waitpid.
 *      Si nadie espera (proceso huérfano), los libera process_kill.
 *   5. Forzar context switch — nunca retorna.
 */
void process_exit(int status)
{
    PCB *p = scheduler_get_running();

    scheduler_unschedule(p);
    p->exit_status = status;
    p->state = ZOMBIE;

    if (p->waiting_me)
    {
        PCB *waiter = p->waiting_me;
        waiter->waiting_for = NULL;
        scheduler_ready(waiter);
    }

    /* Forzar switch: no volvemos a correr sobre este stack. */
    timer_tick();
    while (1)
        ;
}

/*
 * Retorna el PID del proceso actualmente en ejecución.
 */
pid_t process_getpid(void)
{
    return scheduler_get_running()->pid;
}

/*
 * Cede voluntariamente el CPU incrementando times_ran hasta el quantum
 * máximo y disparando un tick sintético.
 */
void process_yield(void)
{
    scheduler_yield();
}

/*
 * Espera a que pid termine y recoge su exit status.
 *
 * Retorna pid en éxito, -1 en cualquier error:
 *   - pid inválido / FREE
 *   - pid == yo mismo
 *   - ya hay otro proceso esperando por pid
 *   - pid fue killed antes o durante la espera
 */
pid_t process_waitpid(pid_t pid, int *ret_out)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;

    PCB *target = &pcbs[pid];
    PCB *me = scheduler_get_running();

    if (target->state == FREE)
        return -1;
    if (target == me)
        return -1;
    if (target->waiting_me != NULL)
        return -1; /* ya tiene waiter */

    /* Si aún no terminó, bloquearse hasta que process_exit nos despierte. */
    if (target->state != ZOMBIE)
    {
        target->waiting_me = me;
        me->waiting_for = target;

        /*
         * scheduler_block hace yield si p == running (que es nuestro caso),
         * así que esta llamada suspende este proceso y transfiere el CPU.
         * Al volver, el target ya está en ZOMBIE (o FREE si fue killed).
         */
        scheduler_block(me);

        me->waiting_for = NULL;

        /* El target pudo haber sido killed mientras esperábamos. */
        if (target->state == FREE)
            return -1;
        if (target->state != ZOMBIE)
            return -1;
    }

    /* Reap: copiar status, liberar recursos, marcar FREE. */
    if (ret_out)
        *ret_out = (int)target->exit_status;

    free_process_resources(target);
    target->state = FREE;
    target->waiting_me = NULL;

    return pid;
}

/*
 * Mata pid incondicionalmente (si killable=1).
 *
 * Diferencias respecto a process_exit:
 *   - El proceso objetivo puede no ser el running.
 *   - Si tenía waiter, lo despertamos con state=FREE (no ZOMBIE), así
 *     waitpid detecta el caso y retorna -1.
 *   - Liberamos recursos inmediatamente (nadie va a hacer waitpid).
 *   - Si matamos al running, forzamos switch.
 */
int process_kill(pid_t pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;

    PCB *p = &pcbs[pid];
    if (p->state == FREE || !p->killable)
        return -1;

    scheduler_unschedule(p);

    /* Despertar al waiter: verá state=FREE y retornará -1. */
    if (p->waiting_me)
    {
        PCB *waiter = p->waiting_me;
        p->waiting_me = NULL;
        waiter->waiting_for = NULL;
        scheduler_ready(waiter);
    }

    /* Limpiar el puntero bidireccional si este estaba esperando a alguien. */
    if (p->waiting_for)
    {
        p->waiting_for->waiting_me = NULL;
        p->waiting_for = NULL;
    }

    /*
     * TODO(F5): si p->blocked_by_sem != -1, remover de la cola del semáforo.
     * TODO(F6): cerrar fds de pipe si corresponde.
     */

    free_process_resources(p);
    p->state = FREE;

    /* Si matamos al proceso running, no podemos volver a su stack. */
    if (scheduler_get_running() == p)
    {
        timer_tick();
        while (1)
            ;
    }

    return 0;
}

/*
 * Cambia la prioridad de pid.
 * Efecto inmediato: el scheduler usa p->priority en el próximo tick.
 */
int process_nice(pid_t pid, priority_t new_pri)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;
    if ((int)new_pri < 0 || new_pri >= MAX_PRIO)
        return -1;

    PCB *p = &pcbs[pid];
    if (p->state == FREE)
        return -1;

    p->priority = new_pri;
    return 0;
}

/*
 * Bloquea pid desde afuera (comando 'block' de la shell).
 * Si pid == running, cede el CPU inmediatamente.
 */
int process_block(pid_t pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;

    PCB *p = &pcbs[pid];
    if (p->state == FREE || p->state == ZOMBIE || p->state == BLOCKED)
        return -1;

    /*
     * scheduler_block hace yield si p == running; si es otro proceso,
     * solo lo mueve a blocked_list sin ceder el CPU.
     */
    scheduler_block(p);
    return 0;
}

/*
 * Desbloquea pid (puede ser cualquier proceso en BLOCKED).
 */
int process_unblock(pid_t pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;

    PCB *p = &pcbs[pid];
    if (p->state != BLOCKED)
        return -1;

    scheduler_ready(p);
    return 0;
}

/*
 * Retorna el estado del proceso pid como entero (pstate_t casteado),
 * o -1 si el pid es inválido.
 */
int process_get_status(pid_t pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;
    return (int)pcbs[pid].state;
}

/* ============================================================
 * F4 — ps
 * ============================================================ */

/*
 * Un proceso es foreground si la shell está bloqueada esperando por él.
 * En F6 esto se extenderá para cubrir el caso pipe: shell espera al lector,
 * que a su vez tiene al escritor como foreground real.
 */
int is_foreground(PCB *p)
{
    PCB *shell = scheduler_get_shell();
    if (!shell)
        return 0;
    /*
     * shell->waiting_for apunta al proceso por el que la shell hizo waitpid.
     * Ese es el foreground actual.
     */
    return (shell->waiting_for == p);
}

/*
 * Construye en el kernel heap un snapshot de todos los procesos no-FREE.
 * El caller libera con process_free_ps() para que mem_free corra en
 * contexto kernel (las syscalls son el único caller esperado).
 */
ProcessInfoList *process_ps(void)
{
    /* Contar procesos activos. */
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (pcbs[i].state != FREE)
            count++;
    }
    if (count == 0)
        return NULL;

    ProcessInfoList *list = (ProcessInfoList *)mem_alloc(sizeof(ProcessInfoList));
    if (!list)
        return NULL;

    list->entries = (ProcessInfo *)mem_alloc(count * sizeof(ProcessInfo));
    if (!list->entries)
    {
        mem_free(list);
        return NULL;
    }
    list->count = count;

    int idx = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        PCB *p = &pcbs[i];
        if (p->state == FREE)
            continue;

        ProcessInfo *info = &list->entries[idx++];
        info->pid = p->pid;
        info->priority = p->priority;
        info->state = p->state;
        info->rsp = p->rsp;
        info->stack_base = p->stack_base;
        info->fds[0] = p->fds[0];
        info->fds[1] = p->fds[1];
        info->fds[2] = p->fds[2];
        info->foreground = is_foreground(p);

        int j = 0;
        while (j < (int)sizeof(info->name) - 1 && p->name[j])
        {
            info->name[j] = p->name[j];
            j++;
        }
        info->name[j] = '\0';
    }

    return list;
}

/*
 * Libera la ProcessInfoList alocada por process_ps().
 */
void process_free_ps(ProcessInfoList *list)
{
    if (!list)
        return;
    if (list->entries)
        mem_free(list->entries);
    mem_free(list);
}