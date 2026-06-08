#include "exceptions.h"
#include "videoDriver.h"

// Mensajes de excepción del procesador x86-64
static const char *exception_messages[] = {
    "Division By Zero",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Invalid Opcode"
};

// Declaraciones externas necesarias
extern uint64_t reg_snapshot[18];
extern uint8_t  snapshot_available;

// Nombres de los registros
static const char* register_names[] = {
    "RAX", "RBX", "RCX", "RDX",
    "RSI", "RDI", "RBP", "RSP",
    "R8",  "R9",  "R10", "R11",
    "R12", "R13", "R14", "R15",
    "RIP", "RFLAGS"
};

// Convierte un uint64_t a cadena hexadecimal (formato "0xNNNNNNNNNNNNNNNN")
static void uint64ToHexKernel(uint64_t value, char* buffer) {
    buffer[0] = '0';
    buffer[1] = 'x';

    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        if (nibble < 10) {
            buffer[17 - i] = '0' + nibble;
        } else {
            buffer[17 - i] = 'A' + (nibble - 10);
        }
    }
    buffer[18] = '\0';
}

void exceptionDispatcher(int exception, uint64_t rsp) {
    (void)rsp;

    clearScreen();

    vd_drawString(0, 0, "*** EXCEPTION: ", 0xFF0000, 2);

    if (exception >= 0 && exception < 7) {
        vd_drawString(0, 34, exception_messages[exception], 0xFF0000, 2);
    } else {
        vd_drawString(0, 34, "Unknown Exception", 0xFF0000, 2);
    }

    int y_offset = 80;

    if (snapshot_available) {
        vd_drawString(0, y_offset, "Register Snapshot:", 0xFF0000, 1);
        y_offset += 20;

        char hexBuffer[19];

        for (int i = 0; i < 18; i++) {
            vd_drawString(10, y_offset, register_names[i], 0xFF0000, 1);
            vd_drawString(70, y_offset, ": ", 0xFF0000, 1);

            uint64ToHexKernel(reg_snapshot[i], hexBuffer);
            vd_drawString(90, y_offset, hexBuffer, 0xFF0000, 1);

            y_offset += 18;
        }
    } else {
        vd_drawString(0, y_offset, "No register snapshot available", 0xFF0000, 1);
        y_offset += 18;
    }

    setCursorPosition(0, y_offset + 10);

    while (1) {
        // Loop infinito para mantener la pantalla de excepción
    }
}
