all: bootloader kernel userland image

# Compilar con el memory manager del buddy system (-DBUDDY_SYSTEM).
# Requiere 'make clean' si ya compilaste sin buddy, porque make no detecta
# que EXTRA_FLAGS cambió (solo compara timestamps de archivos).
buddy:
	cd Bootloader; make all
	cd Kernel; make all EXTRA_FLAGS=-DBUDDY_SYSTEM
	cd Userland; make all
	cd Toolchain; make all
	cd Image; make all

bootloader:
	cd Bootloader; make all

kernel:
	cd Kernel; make all

userland:
	cd Userland; make all

toolchain:
	cd Toolchain; make all

image: kernel bootloader userland toolchain
	cd Image; make all

# Correr en QEMU (requiere acceso gráfico local).
run:
	qemu-system-x86_64 -enable-kvm -m 512 -hda Image/x64BareBonesImage.img \
	    -serial stdio -vga std

clean:
	cd Bootloader; make clean
	cd Image; make clean
	cd Kernel; make clean
	cd Userland; make clean

.PHONY: bootloader kernel buddy userland toolchain all image run clean
