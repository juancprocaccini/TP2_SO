#include "exceptions.h"
#include "videoDriver.h"

static char *exception_messages[][2] = {
    {"Division By Zero", "Error de división por cero"},
    {"Reserved", "Reservado"},
    {"Reserved", "Reservado"},
    {"Reserved", "Reservado"},
    {"Reserved", "Reservado"},
    {"Reserved", "Reservado"},
    {"Invalid Opcode", "Código de operación inválido"}
};

// Declaraciones externas necesarias
extern uint64_t reg_snapshot[18];
extern uint8_t snapshot_available;

// Nombres de los registros
static const char* register_names[] = {
    "RAX", "RBX", "RCX", "RDX", 
    "RSI", "RDI", "RBP", "RSP",
    "R8",  "R9",  "R10", "R11",
    "R12", "R13", "R14", "R15",
    "RIP", "RFLAGS"
};

// Función helper para convertir uint64_t a string hexadecimal
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
    // Limpiar pantalla
    clearScreen();
    
    // Mostrar mensaje de excepción
    vd_drawString(0, 0, "*** EXCEPTION: ", 0xFF0000, 2);
    
    if (exception >= 0 && exception < 7) {
        vd_drawString(0, 34, exception_messages[exception][0], 0xFF0000, 2);
    } else {
        vd_drawString(0, 34, "Unknown Exception", 0xFF0000, 2);
    }
    
    // Espacio antes de los registros
    int y_offset = 80;
    
    // Imprimir registros si hay snapshot disponible
    if (snapshot_available) {
        vd_drawString(0, y_offset, "Register Snapshot:", 0xFF0000, 1);
        y_offset += 20;
        
        char hexBuffer[19];
        
        for (int i = 0; i < 18; i++) {
            // Nombre del registro (ancho de fuente = 8 pixels por caracter con size 1)
            vd_drawString(10, y_offset, register_names[i], 0xFF0000, 1);
            vd_drawString(70, y_offset, ": ", 0xFF0000, 1);
            
            // Valor en hexadecimal
            uint64ToHexKernel(reg_snapshot[i], hexBuffer);
            vd_drawString(90, y_offset, hexBuffer, 0xFF0000, 1);
            
            // Incrementar Y para la siguiente línea (altura de fuente = 16 pixels con size 1)
            y_offset += 18;
        }
    } else {
        vd_drawString(0, y_offset, "No register snapshot available", 0xFF0000, 1);
        y_offset += 18;
    }
    
    // Posicionar el cursor al final del mensaje para continuar usando la terminal
    setCursorPosition(0, y_offset + 10);
    
    // NO hacer halt - permitir que retorne
    // El handler en exceptions.asm hará popState e iretq
}
