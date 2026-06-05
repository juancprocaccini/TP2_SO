#include "semaphore.h"
#include "defs.h"
#include "process.h"
#include "scheduler.h"
#include "queue.h"
#include <stddef.h>

typedef struct
{
    uint64_t value;
    uint8_t lock;       /* xchg-based spinlock */
    uint8_t open_count; /* 0 = slot free */
    queue_t *waiters;   /* FIFO de PCB* usando tu queue.c */
} sem_t;

static sem_t sems[SEM_KERNEL_MAX];

// Declaramos las funciones de Assembly para manejar interrupciones
extern void _cli(void);
extern void _sti(void);
extern uint8_t atomic_xchg(uint8_t *lock, uint8_t val);

static void acquire(uint8_t *lock)
{
    while (atomic_xchg(lock, 1) != 0)
    {
        // Espera ocupada (Spinlock)
    }
}

static void release(uint8_t *lock)
{
    atomic_xchg(lock, 0);
}

static int is_valid_id(int id, int is_kernel)
{
    if (is_kernel)
    {
        return (id >= SEM_USER_MAX && id < SEM_KERNEL_MAX);
    }
    return (id >= 0 && id < SEM_USER_MAX);
}

void ksem_init(void)
{
    for (int i = 0; i < SEM_KERNEL_MAX; i++)
    {
        sems[i].open_count = 0;
        sems[i].lock = 0;
        sems[i].value = 0;
        sems[i].waiters = queue_new(); // Tu constructor de queue.c
    }
}

int ksem_open(int id, uint64_t initial)
{
    if (!is_valid_id(id, 0))
        return -1;

    _cli(); // Protegemos la región crítica a nivel Kernel
    acquire(&sems[id].lock);

    if (sems[id].open_count == 0)
    {
        sems[id].value = initial;
    }
    sems[id].open_count++;

    release(&sems[id].lock);
    _sti();

    return id;
}

int ksem_open_get_id(uint64_t initial)
{
    _cli();
    // Restringido estrictamente al rango de USERLAND (0 a 99) como bien notaste
    for (int i = 0; i < SEM_USER_MAX; i++)
    {
        acquire(&sems[i].lock);
        if (sems[i].open_count == 0)
        {
            sems[i].value = initial;
            sems[i].open_count = 1;
            release(&sems[i].lock);
            _sti();
            return i;
        }
        release(&sems[i].lock);
    }
    _sti();
    return -1;
}

int ksem_wait(int id)
{
    if (id < 0 || id >= SEM_KERNEL_MAX)
        return -1;

    _cli(); // Zona de alta seguridad: apagamos interrupciones
    sem_t *sem = &sems[id];
    acquire(&sem->lock);

    if (sem->open_count == 0)
    {
        release(&sem->lock);
        _sti();
        return -1;
    }

    if (sem->value > 0)
    {
        sem->value--;
        release(&sem->lock);
        _sti();
        return 0;
    }

    // Si no hay recursos, toca bloquearse
    PCB *running = scheduler_get_running();
    queue_enqueue(sem->waiters, running); // Tu función de queue.c

    scheduler_block_no_yield(running);
    process_set_block_by_sem(running->pid, id);

    release(&sem->lock);
    _sti(); // Habilitamos interrupciones JUSTO antes de ceder el control
    scheduler_yield();

    return 0;
}

int ksem_post(int id)
{
    if (id < 0 || id >= SEM_KERNEL_MAX)
        return -1;

    _cli();
    sem_t *sem = &sems[id];
    acquire(&sem->lock);

    if (sem->open_count == 0)
    {
        release(&sem->lock);
        _sti();
        return -1;
    }

    if (queue_is_empty(sem->waiters))
    {
        sem->value++;
    }
    else
    {
        PCB *next_pcb = (PCB *)queue_dequeue(sem->waiters);
        if (next_pcb != NULL)
        {
            process_unblock(next_pcb->pid);
            process_set_block_by_sem(next_pcb->pid, -1);
        }
    }

    release(&sem->lock);
    _sti();

    scheduler_yield(); // Cede la CPU para permitir alternancia justa
    return 0;
}

int ksem_post_no_yield(int id)
{
    if (id < 0 || id >= SEM_KERNEL_MAX)
        return -1;

    _cli();
    sem_t *sem = &sems[id];
    acquire(&sem->lock);

    if (sem->open_count == 0)
    {
        release(&sem->lock);
        _sti();
        return -1;
    }

    if (queue_is_empty(sem->waiters))
    {
        sem->value++;
    }
    else
    {
        PCB *next_pcb = (PCB *)queue_dequeue(sem->waiters);
        if (next_pcb != NULL)
        {
            process_unblock(next_pcb->pid);
            process_set_block_by_sem(next_pcb->pid, -1);
        }
    }

    release(&sem->lock);
    _sti();
    return 0;
}

int ksem_close(int id)
{
    if (id < 0 || id >= SEM_KERNEL_MAX)
        return -1;

    _cli();
    acquire(&sems[id].lock);

    if (sems[id].open_count > 0)
    {
        sems[id].open_count--;
        if (sems[id].open_count == 0)
        {
            // Si nadie más lo usa, vaciamos la cola de forma segura
            while (!queue_is_empty(sems[id].waiters))
            {
                queue_dequeue(sems[id].waiters);
            }
        }
    }

    release(&sems[id].lock);
    _sti();
    return 0;
}

// Función auxiliar indispensable para que process_kill pueda limpiar al proceso si muere bloqueado
int ksem_remove_waiter(int id, void *pcb)
{
    if (id < 0 || id >= SEM_KERNEL_MAX)
        return -1;
    return queue_remove(sems[id].waiters, pcb); // Tu función de queue.c
}