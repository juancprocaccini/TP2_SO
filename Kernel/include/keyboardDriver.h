#include <stdint.h>
#include <stddef.h>


#ifndef KEYBOARD_H // ya esta definido keyboard.h??
#define KEYBOARD_H // si no, incluilo ahora (evita incluir mas de una vez)

#define BIT_SCANCODE_UP 0b10000000 //bit que indica si la tecla fue soltada o presionada
#define BIT_SCANCODE_DOWN 0b00000000 

#define KEYBOARD_DATA_PORT    0x60 //aca se lee el scancode
#define KEYBOARD_STATUS_PORT  0x64  //aca se lee el estado del teclado
#define KEYBOARD_COMMAND_PORT 0x64 //aca se envian comandos al teclado(comandos como activar/desactivar el teclado, resetearlo, etc)

#define SCANCODE_RELEASED     0x80 //para saber si una tecla fue soltada
#define IS_KEY_RELEASED(sc)   ((sc) & SCANCODE_RELEASED) //macro que verifica si la tecla fue soltada
#define GET_KEY_CODE(sc)      ((sc) & 0x7F) //macro que obtiene el codigo de la tecla sin el bit de presion/soltado (apaga el bit mas significativo que es el de presion/soltado)


void kbd_init(void); //inicializa el teclado (configura el PIC y el IDT)
void kbd_handler(uint8_t scancode); //rutina de atencion a la interrupcion del teclado->se ejecuta cada vez que se presiona una tecla
char kbd_scancode_to_ascii(uint8_t scancode, uint8_t modifiers); //convierte un scancode a un caracter ascii (si es posible), recibe el scancode y los modificadores (shift, ctrl, alt)
const char* kbd_scancode_to_name(uint8_t scancode); //convierte un scancode a un nombre descriptivo (ej: "A", "ENTER", "SPACE", etc)

uint8_t kbd_get_modifiers(void);     // me dice cuales modificadores estan activos (shift, ctrl, alt)
uint8_t kbd_is_shift_pressed(void);  // Shift activado?
uint8_t kbd_is_ctrl_pressed(void);   // Ctrl activado?
uint8_t kbd_is_alt_pressed(void);    // Alt activado?
uint8_t kbd_is_caps_lock_on(void);   // Caps Lock activado?

uint8_t kbd_is_key_pressed(uint8_t scancode); //me dice si una tecla esta presionada (recibe el scancode de la tecla)
uint32_t kbd_read_chars(char* buffer, uint32_t max_chars); //lee caracteres del buffer del teclado (almacena en buffer hasta max_chars), devuelve la cantidad de caracteres leidos
uint32_t kbd_available_chars(void); //sin esto el juego se queda colgado hasta que se presiona una tecla, devuelve la cantidad de caracteres disponibles en el buffer del teclado
void kbd_clear_buffer(void); //limpia el buffer del teclado
char kbd_buffer_get(void); //obtiene un caracter del buffer del teclado

// Estructura para registro de CPU
typedef struct{
    uint64_t name;  // Puntero al nombre del registro
    uint64_t data;  // Valor del registro
} registered_reg;

// Función para obtener snapshot de registros
registered_reg* load_data(void);
registered_reg* load_names(void);

#endif