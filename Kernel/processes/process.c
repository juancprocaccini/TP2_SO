#include <process.h>
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
        for (int i = 0; i < argc; i++) {
            int len = strlen(argv[i]);
            new_argv[i] = (char *)mem_alloc(len + 1);
            strcpy(new_argv[i], argv[i]);
        }
        new_argv[argc] = NULL;
    }

    PCB *pcb = &pcbs[pid];
    pcb->pid = pid;
    pcb->state = READY;
    pcb->priority = pri;
    pcb->stack_base = (uint64_t)stack;
    pcb->argc = argc;
    pcb->argv = new_argv;
    pcb->killable = killable;
    pcb->waiting_me = NULL;
    pcb->waiting_for = NULL;
    pcb->blocked_by_sem = -1;
    
    if (fds) {
        pcb->fds[0] = fds[0];
        pcb->fds[1] = fds[1];
        pcb->fds[2] = fds[2];
    } else {
        pcb->fds[0] = 0;
        pcb->fds[1] = 1;
        pcb->fds[2] = 2;
    }

    /* * Forjado de Stack.
     * Simula el marco de una interrupción para que iretq restaure el contexto.
     * El orden empujado desciende en memoria y es inverso al popState.
     */
    uint64_t stack_top = (uint64_t)stack + STACK_SIZE;
    uint64_t *stack_frame = (uint64_t *)stack_top;

    *(--stack_frame) = 0x00;                        /* SS */
    *(--stack_frame) = stack_top;                   /* RSP original */
    *(--stack_frame) = 0x202;                       /* RFLAGS (IF=1) */
    *(--stack_frame) = 0x08;                        /* CS */
    *(--stack_frame) = (uint64_t)process_wrapper;   /* RIP */
    
    *(--stack_frame) = 0;                           /* R15 */
    *(--stack_frame) = 0;                           /* R14 */
    *(--stack_frame) = 0;                           /* R13 */
    *(--stack_frame) = 0;                           /* R12 */
    *(--stack_frame) = 0;                           /* R11 */
    *(--stack_frame) = 0;                           /* R10 */
    *(--stack_frame) = 0;                           /* R9 */
    *(--stack_frame) = 0;                           /* R8 */
    *(--stack_frame) = (uint64_t)new_argv;          /* RSI */
    *(--stack_frame) = (uint64_t)rip;               /* RDI */
    *(--stack_frame) = 0;                           /* RBP */
    *(--stack_frame) = (uint64_t)argc;              /* RDX */
    *(--stack_frame) = (uint64_t)pid;               /* RCX */
    *(--stack_frame) = 0;                           /* RBX */
    *(--stack_frame) = 0;                           /* RAX */

    pcb->rsp = (uint64_t)stack_frame;

    return pid;
}