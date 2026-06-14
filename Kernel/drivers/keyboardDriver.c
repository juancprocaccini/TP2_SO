#include "keyboardDriver.h"
#include "videoDriver.h"
#include "scheduler.h"
#include <stdint.h>
#include <stddef.h>

extern void _cli(void);
extern void _sti(void);

// Scancodes de teclas modificadoras
#define L_SHIFT_SCANCODE    0x2A
#define R_SHIFT_SCANCODE    0x36
#define CTRL_SCANCODE       0x1D
#define ALT_SCANCODE        0x38
#define CAPS_LOCK_SCANCODE  0x3A

static char kbd_buffer[256];
static size_t kbd_buffer_start = 0;
static size_t kbd_buffer_end   = 0;
static size_t kbd_buffer_count = 0;

/* Proceso bloqueado esperando input de teclado (NULL si nadie espera) */
static PCB *kbd_waiting_pcb = NULL;

/* Modo canónico: líneas completas (o EOFs) pendientes de consumir */
static int lines_ready = 0;
/* Longitud de la línea parcial en edición (protege el prompt del backspace) */
static int line_len    = 0;

/* Máximo de caracteres por línea antes de Enter (una menos que el buffer) */
#define KBD_LINE_MAX  255

static uint8_t extended_scancode = 0;

// Estado de teclas modificadoras
static uint8_t kbd_shift_pressed = 0;
static uint8_t kbd_ctrl_pressed  = 0;
static uint8_t kbd_alt_pressed   = 0;
static uint8_t kbd_caps_lock     = 0;

// Tabla ASCII normal
static const char kbd_ascii_table[128] = {
    /* 0x00 */ 0,   0,   '1', '2', '3', '4', '5', '6',
    /* 0x08 */ '7', '8', '9', '0', '-', '=', '\b', '\t',
    /* 0x10 */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    /* 0x18 */ 'o', 'p', '[', ']', '\n', 0,   'a', 's',
    /* 0x20 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    /* 0x28 */ '\'', '`', 0,   '\\','z', 'x', 'c', 'v',
    /* 0x30 */ 'b', 'n', 'm', ',', '.', '/', 0,   0,
    /* 0x38 */ 0,   ' ', 0,   0,   0,   0,   0,   0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

// Tabla ASCII con Shift activo
static const char kbd_shift_ascii_table[128] = {
    /* 0x00 */ 0,   0,   '!', '@', '#', '$', '%', '^',
    /* 0x08 */ '&', '*', '(', ')', '_', '+', '\b', '\t',
    /* 0x10 */ 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    /* 0x18 */ 'O', 'P', '{', '}', '\n', 0,   'A', 'S',
    /* 0x20 */ 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    /* 0x28 */ '\"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    /* 0x30 */ 'B', 'N', 'M', '<', '>', '?', 0,   0,
    /* 0x38 */ 0,   ' ', 0,   0,   0,   0,   0,   0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

// --- Buffer circular ---

static void kbd_buffer_put(char c) {
    /* Permitimos c==0 (EOF vía Ctrl+D); el filtro original excluía null chars
     * de teclas sin mapeo, pero esos nunca llegan acá porque kbd_handler
     * los descarta antes. El 0 se inyecta sólo desde Ctrl+D. */
    if (kbd_buffer_count < sizeof(kbd_buffer)) {
        kbd_buffer[kbd_buffer_end] = c;
        kbd_buffer_end = (kbd_buffer_end + 1) % sizeof(kbd_buffer);
        kbd_buffer_count++;
    }
}

static void kbd_wake_waiter(void) {
    if (kbd_waiting_pcb != NULL) {
        PCB *p = kbd_waiting_pcb;
        kbd_waiting_pcb = NULL;
        scheduler_ready(p);
    }
}

char kbd_buffer_get(void) {
    if (kbd_buffer_count == 0) {
        return 0;
    }
    char c = kbd_buffer[kbd_buffer_start];
    kbd_buffer_start = (kbd_buffer_start + 1) % sizeof(kbd_buffer);
    kbd_buffer_count--;
    return c;
}

// Devuelve el carácter correspondiente al scancode según los modificadores activos
static char get_char_with_modifiers(uint8_t key_code) {
    if (kbd_shift_pressed || kbd_caps_lock) {
        return kbd_shift_ascii_table[key_code];
    }
    return kbd_ascii_table[key_code];
}

// --- API pública ---

void kbd_init(void) {
    kbd_clear_buffer();
    extended_scancode = 0;
    kbd_shift_pressed = 0;
    kbd_ctrl_pressed  = 0;
    kbd_alt_pressed   = 0;
    kbd_caps_lock     = 0;
}

void kbd_handler(uint8_t scancode) {
    if (scancode == 0xE0) {
        extended_scancode = 1;
        return;
    }

    uint8_t key_code    = GET_KEY_CODE(scancode);
    uint8_t is_released = IS_KEY_RELEASED(scancode);

    // Actualizar estado de teclas modificadoras
    switch (key_code) {
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
                kbd_caps_lock = !kbd_caps_lock;
            }
            return;
        default:
            break;
    }

    // Ignorar eventos de tecla soltada para caracteres normales
    if (is_released) {
        return;
    }

    /* Ctrl+D → EOF: mueve el cursor a nueva línea, termina la línea parcial */
    if (kbd_ctrl_pressed && key_code == 0x20) {  /* 0x20 = scancode de 'd' */
        newLine();
        kbd_buffer_put(0);
        line_len = 0;
        lines_ready++;
        kbd_wake_waiter();
        return;
    }

    char letra = get_char_with_modifiers(key_code);
    if (letra == 0) return;     /* tecla sin mapeo ASCII */

    if (letra == '\b') {
        /* Backspace: solo borra dentro de la línea actual, no toca líneas anteriores */
        if (line_len > 0) {
            kbd_buffer_end = (kbd_buffer_end + sizeof(kbd_buffer) - 1) % sizeof(kbd_buffer);
            kbd_buffer_count--;
            line_len--;
            deleteChar();
        }
    } else if (letra == '\n') {
        kbd_buffer_put('\n');
        line_len = 0;
        lines_ready++;
        newLine();
        kbd_wake_waiter();
    } else {
        /* Carácter imprimible: eco inmediato, encolar, no despertar.
         * Al llegar al tope se ignoran los chars entrantes hasta Enter. */
        if (line_len >= KBD_LINE_MAX)
            return;
        vPutChar((uint64_t)(unsigned char)letra, 0xFFFFFF);
        kbd_buffer_put(letra);
        line_len++;
    }
}

void kbd_clear_buffer(void) {
    kbd_buffer_start = 0;
    kbd_buffer_end   = 0;
    kbd_buffer_count = 0;
    lines_ready      = 0;
    line_len         = 0;
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
            break;
        }
        buffer[chars_read] = c;
        chars_read++;
    }
    return chars_read;
}

/*
 * stdin_read — lectura bloqueante de teclado.
 * Bloquea el proceso si el buffer está vacío, se despierta cuando llega un char.
 * Lee hasta n bytes. Se detiene antes si llega '\n' o EOF (byte 0).
 * Retorna 0 si el primer carácter fue EOF (Ctrl+D), count en cualquier otro caso.
 */
int stdin_read(char *buf, int n) {
    if (n <= 0) return 0;

    /* Modo canónico: bloquear hasta que haya una línea completa o EOF */
    while (1) {
        _cli();
        if (lines_ready > 0) {
            _sti();
            break;
        }
        kbd_waiting_pcb = scheduler_get_running();
        scheduler_block_no_yield(kbd_waiting_pcb);
        _sti();
        scheduler_yield();
    }

    /* Leer hasta n bytes, parando en '\n' o EOF (byte 0) */
    int count = 0;
    while (count < n) {
        _cli();
        if (kbd_buffer_count == 0) { _sti(); break; }
        char c = kbd_buffer_get();
        _sti();
        if (c == 0) {
            lines_ready--;
            break;      /* EOF: retornar lo acumulado (0 si es el primer char) */
        }
        buf[count++] = c;
        if (c == '\n') {
            lines_ready--;
            break;
        }
    }
    return count;
}
