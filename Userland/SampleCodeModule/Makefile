include ../Makefile.inc
GCCFLAGS += -I../include

MODULE=0000-sampleCodeModule.bin
SOURCES=$(wildcard [^_]*.c)
USRLIB_ASM=../include/usrlib.asm
USRLIB_OBJ=usrlib.o

all: $(MODULE)

$(USRLIB_OBJ): $(USRLIB_ASM)
	nasm -felf64 $(USRLIB_ASM) -o $(USRLIB_OBJ)

$(MODULE): $(SOURCES) $(USRLIB_OBJ)
	$(GCC) $(GCCFLAGS) -T sampleCodeModule.ld _loader.c $(SOURCES) $(USRLIB_OBJ) -o ../$(MODULE)

clean:
	rm -rf *.o

.PHONY: all clean
