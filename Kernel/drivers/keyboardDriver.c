#include "keyboardDriver.h"
#include <stdint.h>
#include <stddef.h>
#include "videoDriver.h"

// Scancodes de teclas especiales
#define L_SHIFT_SCANCODE    0x2A
#define R_SHIFT_SCANCODE    0x36
#define L_SHIFT_RELEASE     0xAA
#define R_SHIFT_RELEASE     0xB6
#define CTRL_SCANCODE       0x1D
#define CTRL_RELEASE        0x9D
#define ALT_SCANCODE        0x38
#define ALT_RELEASE         0xB8
#define CAPS_LOCK_SCANCODE  0x3A



static char kbd_buffer[256]; //buffer circular para almacenar los caracteres presionados
static size_t kbd_buffer_start = 0; //indice del primer caracter en el buffer
static size_t kbd_buffer_end = 0;   //indice del ultimo caracter en el buffer
static size_t kbd_buffer_count = 0;  //cantidad de caracteres en el buffer

static uint8_t extended_scancode = 0;

// Estados de teclas modificadoras
static uint8_t kbd_shift_pressed = 0;
static uint8_t kbd_ctrl_pressed = 0;
static uint8_t kbd_alt_pressed = 0;
static uint8_t kbd_caps_lock = 0;

// Reemplazar la tabla ASCII normal
static const char kbd_ascii_table[128] = {
    /* 0x00 */ 0,   0,   '1', '2', '3', '4', '5', '6',
    /* 0x08 */ '7', '8', '9', '0', '-', '=', '\b', '\t',
    /* 0x10 */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    /* 0x18 */ 'o', 'p', '[', ']', '\n', 0,   'a', 's',
    /* 0x20 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    /* 0x28 */ '\'', '`', 0,   '\\','z', 'x', 'c', 'v',
    /* 0x30 */ 'b', 'n', 'm', ',', '.', '/', 0,   0,
    /* 0x38 */ 0,   ' ', 0,   0,   0,   0,   0,   0,
    /* resto 0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

// Reemplazar la tabla ASCII con SHIFT
static const char kbd_shift_ascii_table[128] = {
    /* 0x00 */ 0,   0,   '!', '@', '#', '$', '%', '^',
    /* 0x08 */ '&', '*', '(', ')', '_', '+', '\b', '\t',
    /* 0x10 */ 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    /* 0x18 */ 'O', 'P', '{', '}', '\n', 0,   'A', 'S',
    /* 0x20 */ 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    /* 0x28 */ '\"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    /* 0x30 */ 'B', 'N', 'M', '<', '>', '?', 0,   0,
    /* 0x38 */ 0,   ' ', 0,   0,   0,   0,   0,   0,
    /* resto 0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

//funciones internas del buffer

static void kbd_buffer_put(char c) {
    if ((kbd_buffer_count < sizeof(kbd_buffer)) && c != 0) { //si no esta lleno
        kbd_buffer[kbd_buffer_end] = c; //agrego el caracter al final
        kbd_buffer_end = (kbd_buffer_end + 1) % sizeof(kbd_buffer); //muevo el indice del final
        kbd_buffer_count++; //incremento la cantidad
     }
}

char kbd_buffer_get(void) {
    if (kbd_buffer_count == 0) {
        return 0; //buffer vacio
    }
    char c = kbd_buffer[kbd_buffer_start]; //obtengo el caracter del inicio
    kbd_buffer_start = (kbd_buffer_start + 1) % sizeof(kbd_buffer); //muevo el indice del inicio
    kbd_buffer_count--; //decremento la cantidad
    return c;
}

// Función para obtener el carácter considerando modificadores
static char get_char_with_modifiers(uint8_t key_code) {
    char letra;
    
    // Determinar qué tabla usar
    if (kbd_shift_pressed || kbd_caps_lock) {
        letra = kbd_shift_ascii_table[key_code];
    } else {
        letra = kbd_ascii_table[key_code];
    }
    
    return letra;
}


//funciones publicas

void kbd_init(void) {
    kbd_clear_buffer();
    extended_scancode = 0;
    kbd_shift_pressed = 0;
    kbd_ctrl_pressed = 0;
    kbd_alt_pressed = 0;
    kbd_caps_lock = 0;
}



void kbd_handler(uint8_t scancode) {
    if(scancode == 0xE0) {
        extended_scancode = 1;
        return;
    }

    uint8_t key_code = GET_KEY_CODE(scancode);
    uint8_t is_released = IS_KEY_RELEASED(scancode);

    // Manejar teclas especiales (modificadores)
    switch(key_code) {
        case L_SHIFT_SCANCODE:
        case R_SHIFT_SCANCODE:
            kbd_shift_pressed = !is_released;
            return;
            
        case CTRL_SCANCODE:
            kbd_ctrl_pressed = !is_released;
            return;
            
        case ALT_SCANCODE:
            kbd_alt_pressed = !is_released;
            return;
            
        case CAPS_LOCK_SCANCODE:
            if (!is_released) {
                kbd_caps_lock = !kbd_caps_lock;  // Toggle caps lock
            }
            return;
            
        default:
            break;
    }

    // Si es tecla liberada, no procesar como carácter
    if(is_released) {
        return;
    }

    // Obtener carácter considerando modificadores
    char letra = get_char_with_modifiers(key_code);

    // Si es Ctrl + algo, manejarlo como comando especial
    if (kbd_ctrl_pressed && letra != 0) {
        // Por ejemplo, Ctrl+C podría ser 0x03
        // Pero por ahora solo ponemos el carácter normal
        kbd_buffer_put(letra);
    } 
    else if (letra != 0) {
        kbd_buffer_put(letra);
    }
}


void kbd_clear_buffer(void) {
    kbd_buffer_start = 0;
    kbd_buffer_end = 0;
    kbd_buffer_count = 0;
    for (size_t i = 0; i < sizeof(kbd_buffer); i++) {
        kbd_buffer[i] = 0;
    }
}

uint32_t kbd_available_chars(void) {
    return kbd_buffer_count;
}

uint32_t kbd_read_chars(char* buffer, uint32_t max_chars) {
    uint32_t chars_read = 0;
    while (chars_read < max_chars && kbd_buffer_count > 0) {
        char c = kbd_buffer_get();
        if (c == 0) {
            break; //buffer vacio
        }
        buffer[chars_read] = c;
        chars_read++;
    }
    return chars_read;
}

// Funciones para consultar estado de modificadores
uint8_t kbd_is_shift_pressed(void) {
    return kbd_shift_pressed;
}

uint8_t kbd_is_ctrl_pressed(void) {
    return kbd_ctrl_pressed;
}

uint8_t kbd_is_alt_pressed(void) {
    return kbd_alt_pressed;
}

uint8_t kbd_is_caps_lock_on(void) {
    return kbd_caps_lock;
}