#include <stdint.h>
#include <lib.h>
#include <idtLoader.h>
#include "keyboardDriver.h"

extern uint8_t bss;
extern uint8_t endOfKernel;
extern void asm_sti(void);

static const uint64_t PageSize = 0x1000;
static void * const sampleCodeModuleAddress = (void*)0x400000;

typedef int (*EntryPoint)();

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

void * initializeKernelBinary() {
	clearBSS(&bss, &endOfKernel - &bss);
	return getStackBase();
}

int main() {
	idtLoader();
	kbd_init();
	asm_sti();

	((EntryPoint)sampleCodeModuleAddress)();

	while (1);
	return 0;
}
