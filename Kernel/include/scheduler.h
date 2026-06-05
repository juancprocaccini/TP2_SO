#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <process.h>

void     scheduler_init(pid_t shell_pid, pid_t idle_pid);
uint64_t scheduler(uint64_t current_rsp);

int      scheduler_ready(PCB *p);
void     scheduler_block(PCB *p);
void     scheduler_block_no_yield(PCB *p);
void     scheduler_yield(void);
void     scheduler_unschedule(PCB *p);
PCB *    scheduler_get_running(void);
int      is_foreground(pid_t pid);



#endif