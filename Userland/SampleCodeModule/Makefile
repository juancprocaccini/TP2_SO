include ../Makefile.inc
GCCFLAGS += -I../include

MODULE=0000-sampleCodeModule.bin

SOURCES=$(wildcard [^_]*.c)
OBJECTS=$(SOURCES:.c=.o)

USRLIB_ASM=../include/usrlib.asm
USRLIB_ASM_OBJ=usrlib.o

all: $(MODULE)

$(USRLIB_ASM_OBJ): $(USRLIB_ASM)
	nasm -felf64 $(USRLIB_ASM) -o $(USRLIB_ASM_OBJ)

_loader.o: _loader.c
	$(GCC) $(GCCFLAGS) -c _loader.c -o _loader.o

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

$(MODULE): _loader.o $(USRLIB_ASM_OBJ) $(OBJECTS)
	$(LD) -T sampleCodeModule.ld _loader.o --start-group $(OBJECTS) $(USRLIB_ASM_OBJ) --end-group -o ../$(MODULE)

clean:
	rm -rf *.o ../$(MODULE)

.PHONY: all clean
