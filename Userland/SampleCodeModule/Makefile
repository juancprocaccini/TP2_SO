include ../Makefile.inc
GCCFLAGS += -I../include

MODULE=0000-sampleCodeModule.bin
SOURCES=$(wildcard [^_]*.c)
OBJECTS=$(SOURCES:.c=.o)

USRLIB=usrlib/usrlib.c
USRLIB_OBJ=usrlib.o
USRLIB_ASM=../include/usrlib.asm

all: $(MODULE)

$(USRLIB_OBJ): $(USRLIB_ASM)
	nasm -felf64 $(USRLIB_ASM) -o $(USRLIB_OBJ)

_loader.o: _loader.c
	$(GCC) $(GCCFLAGS) -c _loader.c -o _loader.o

#Regla genérica para compilar CUALQUIER archivo .c a .o

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

$(MODULE): _loader.o $(USRLIB_OBJ) $(OBJECTS)
	$(LD) -T sampleCodeModule.ld _loader.o $(OBJECTS) $(USRLIB_OBJ) -o ../$(MODULE)

clean:
	rm -rf *.o ../$(MODULE)

.PHONY: all clean
