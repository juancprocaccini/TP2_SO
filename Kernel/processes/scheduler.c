#include <scheduler.h>
#include <list.h>
#include <stddef.h>

extern void timer_tick(void);

static list_t  *ready_list;
static list_t  *blocked_list;
static PCB     *running;
static uint64_t times_ran;
static PCB     *idle_pcb;
static PCB     *shell_pcb;
static int      lists_ready = 0;
static int      initialized = 0;

/*
 * Inicialización perezosa de las listas. Permite que process_create encole
 * procesos vía scheduler_ready antes de que scheduler_init corra, sin obligar
 * a kernel.c a llamar a dos rutinas distintas.
 */
static int ensure_lists(void) {
    if (lists_ready) return 0;
    ready_list = list_new(NULL);
    if (!ready_list) return -1;
    blocked_list = list_new(NULL);
    if (!blocked_list) {
        list_free(ready_list);
        ready_list = NULL;
        return -1;
    }
    lists_ready = 1;
    return 0;
}

void scheduler_init(pid_t shell_pid, pid_t idle_pid) {
    ensure_lists();
    shell_pcb = process_get(shell_pid);
    idle_pcb = process_get(idle_pid);
    running = NULL;
    times_ran = 0;

    /*
     * Idle no compite en el round-robin: corre sólo cuando ready_list queda
     * vacía. process_create lo dejó encolado, así que lo sacamos acá.
     */
    if (idle_pcb) {
        list_remove(ready_list, idle_pcb);
    }
    initialized = 1;
}

uint64_t scheduler(uint64_t current_rsp) {
    if (!initialized) {
        return current_rsp;
    }

    if (running != NULL) {
        running->rsp = current_rsp;
    }

    if (list_is_empty(ready_list)) {
        running = idle_pcb;
        return idle_pcb->rsp;
    }

    if (running == NULL) {
        running = (PCB *)list_next(ready_list);
        return running->rsp;
    }

    if (times_ran >= running->priority || running->state != READY) {
        times_ran = 0;
        running = (PCB *)list_next(ready_list);
        return running->rsp;
    }

    times_ran++;
    return current_rsp;
}

int scheduler_ready(PCB *p) {
    if (!p) return -1;
    if (ensure_lists() < 0) return -1;
    list_remove(blocked_list, p);
    if (list_add(ready_list, p) < 0) return -1;
    p->state = READY;
    return 0;
}

void scheduler_block_no_yield(PCB *p) {
    p->state = BLOCKED;
    list_remove(ready_list, p);
    list_add(blocked_list, p);
}

void scheduler_block(PCB *p) {
    scheduler_block_no_yield(p);
    if (p == running) {
        timer_tick();
    }
}

void scheduler_yield(void) {
    if (running) {
        times_ran = running->priority;
    }
    timer_tick();
}

void scheduler_unschedule(PCB *p) {
    list_remove(ready_list, p);
    list_remove(blocked_list, p);
}

PCB *scheduler_get_running(void) {
    return running;
}

PCB *scheduler_get_shell(void) { return shell_pcb; }