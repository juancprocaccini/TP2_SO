OUTPUT_FORMAT("binary")
ENTRY(_start)
SECTIONS
{
    .text 0x400000 :
    {
        */_loader.o(.text)  /* <-- ESTA ES LA CLAVE: _loader.o SIEMPRE PRIMERO */
        *(.text*)
        *(.rodata*)
    }
    .data ALIGN(0x1000) :
    {
        *(.data*)
    }
    .bss ALIGN(0x1000) :
    {
        bss = .;
        *(.bss*)
    }
    endOfBinary = .;
}