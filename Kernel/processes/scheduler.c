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
static int      initialized = 0;

void scheduler_init(pid_t shell_pid, pid_t idle_pid) {
    ready_list = list_new(NULL);
    blocked_list = list_new(NULL);
    shell_pcb = process_get(shell_pid);
    idle_pcb = process_get(idle_pid);
    running = NULL;
    times_ran = 0;
    initialized = 1;

    scheduler_ready(shell_pcb);
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

void scheduler_ready(PCB *p) {
    p->state = READY;
    list_remove(blocked_list, p);
    list_add(ready_list, p);
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