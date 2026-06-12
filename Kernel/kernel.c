#include <stdint.h>
#include <stddef.h>
#include <lib.h>
#include "idtLoader.h"
#include "keyboardDriver.h"
#include <mem.h>
#include <mem_user.h>
#include <defs.h>
#include "process.h"
#include "scheduler.h"
#include "semaphore.h"
#include "pipe.h"

extern uint8_t bss;
extern uint8_t endOfKernel;
extern uint8_t endOfKernelBinary;
extern void _sti(void);
extern void timer_tick(void);

static const uint64_t PageSize = 0x1000;
static void * const sampleCodeModuleAddress = (void*)0x400000;

void clearBSS(void * bssAddress, uint64_t bssSize) {
    memset(bssAddress, 0, bssSize);
}

void * getStackBase() {
    return (void*)(
        (uint64_t)&endOfKernel
        + PageSize * 8
        - sizeof(uint64_t)
    );
}


static void loadModules(void) {
    uint8_t *ptr = &endOfKernelBinary;

    int32_t count = *(int32_t *)ptr;
    ptr += sizeof(int32_t);

    if (count > 0) {
        uint32_t size = *(uint32_t *)ptr;
        ptr += sizeof(uint32_t);
        memcpy(sampleCodeModuleAddress, ptr, size);
    }
}

void *initializeKernelBinary()
{
    loadModules();
    clearBSS(&bss, &endOfKernel - &bss);
    return getStackBase();
}

static int idle_process(char **argv, int argc) {
    while (1) {
        __asm__ volatile("hlt");
    }
    return 0;
}

int main() {
    idtLoader();
    kbd_init();
    
    user_mem_init((void *)USER_HEAP_START, USER_HEAP_SIZE);
    mem_init((void *)KERNEL_HEAP_START, KERNEL_HEAP_SIZE);

    char *shell_argv[] = {"shell"};
    pid_t shell_pid = process_create((entry_t)sampleCodeModuleAddress, MEDIUM, 1, shell_argv, 1, NULL);
    pid_t idle_pid  = process_create(idle_process, LOW, 0, NULL, 0, NULL);

    scheduler_init(shell_pid, idle_pid);
    ksem_init();
    pipe_init();

    _sti();

    /* * Forzamos una interrupción por software (int 0x20) para ceder el control
     * al scheduler. A partir de este punto, main() no vuelve a ejecutar.
     */
    timer_tick();

    while (1);
    return 0;
}