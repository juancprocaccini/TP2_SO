#include <stdint.h>
#include <lib.h>
#include <idtLoader.h>
#include "keyboardDriver.h"

extern uint8_t bss;
extern uint8_t endOfKernel;
extern uint8_t endOfKernelBinary;
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

// El ModulePacker escribe tras el binario del kernel:
//   int32_t  count          (cantidad de módulos)
//   uint32_t size_module_0  (bytes del módulo 0)
//   <binario módulo 0>
//   ...
// endOfKernelBinary apunta justo al byte que sigue al .data del kernel,
// que es donde el packer empieza a escribir sus datos.
static void loadModules(void) {
	uint8_t *ptr = &endOfKernelBinary;

	int32_t count = *(int32_t *)ptr;
	ptr += sizeof(int32_t);

	// Módulo 0: sampleCodeModule.bin — linkeado para correr en 0x400000
	if (count > 0) {
		uint32_t size = *(uint32_t *)ptr;
		ptr += sizeof(uint32_t);
		memcpy(sampleCodeModuleAddress, ptr, size);
	}
}

int main() {
	loadModules();
	idtLoader();
	kbd_init();
	asm_sti();

	((EntryPoint)sampleCodeModuleAddress)();

	while (1);
	return 0;
}
