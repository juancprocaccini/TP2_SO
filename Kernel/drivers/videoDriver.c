#include <videoDriver.h>
#include <font.h>
#include <lib.h>

#define ENABLED 1
#define DISABLED 0
#define DEFAULT_COLOR 0x000000
#define SCREEN_BUFFER_SIZE 4096  // Buffer para almacenar caracteres
static uint32_t cursorX = 0;
static uint32_t cursorY = 0;

// Estructura para almacenar caracteres con su color Y tamaño
typedef struct {
	char ch;
	uint32_t color;
	uint8_t size;  // ← NUEVO: guardar el tamaño con el que se dibujó
} CharEntry;

// Constantes para usar en kernel.c

static uint8_t size = 1;
static CharEntry screenBuffer[SCREEN_BUFFER_SIZE];
static int bufferIndex = 0;

struct vbe_mode_info_structure {
	uint16_t attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	uint8_t window_a;			// deprecated
	uint8_t window_b;			// deprecated
	uint16_t granularity;		// deprecated; used while calculating bank numbers
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
	uint16_t pitch;			// number of bytes per horizontal line
	uint16_t width;			// width in pixels
	uint16_t height;			// height in pixels
	uint8_t w_char;			// unused...
	uint8_t y_char;			// ...
	uint8_t planes;
	uint8_t bpp;			// bits per pixel in this mode
	uint8_t banks;			// deprecated; total number of banks in this mode
	uint8_t memory_model;
	uint8_t bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	uint8_t image_pages;
	uint8_t reserved0;
 
	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;
	uint8_t direct_color_attributes;
 
	uint32_t framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	uint8_t reserved1[206];
} __attribute__ ((packed));

typedef struct vbe_mode_info_structure * VBEInfoPtr;

VBEInfoPtr VBE_mode_info = (VBEInfoPtr) 0x0000000000005C00;

/// FUNCION DEBUG ///

void debugPrintHex(const char *mensaje, uint64_t value)
{
	char buffer[25];
	int index = 23;
	buffer[24] = '\0';
	buffer[index--] = '\n'; // Salto de línea al final

	if (value == 0)
	{
		buffer[index--] = '0';
	}
	else
	{
		while (value > 0 && index >= 2)
		{
			int rem = value % 16;
			if (rem < 10)
				buffer[index--] = '0' + rem;
			else
				buffer[index--] = 'A' + (rem - 10);
			value /= 16;
		}
	}
	buffer[index--] = 'x';
	buffer[index] = '0';

	// Imprimir el mensaje en Amarillo y el número en Rojo
	vprintString(mensaje, 0xFFFF00);
	vprintString(&buffer[index], 0xFF0000);
}

/// FIN FUNCION DEVBUG	///

void putPixel(uint64_t hexColor, uint64_t x, uint64_t y) {
    uint8_t * framebuffer = (uint8_t *) (uint64_t) VBE_mode_info->framebuffer;
    uint64_t offset = (x * ((VBE_mode_info->bpp)/8)) + (y * VBE_mode_info->pitch);
   	framebuffer[offset]     =  (hexColor) & 0xFF;
    framebuffer[offset+1]   =  (hexColor >> 8) & 0xFF; 
    framebuffer[offset+2]   =  (hexColor >> 16) & 0xFF;
}

void moveCursorLeft(){
	uint64_t spaceX = CHAR_WIDTH*size;
	uint64_t spaceY = CHAR_HEIGHT*size;
	if(cursorX >= spaceX){
	cursorX -= spaceX;
	}
	else{
		if(cursorY >= spaceY){
			cursorY -= spaceY;
			cursorX = (VBE_mode_info->width/spaceX - 1)* spaceX;
		}
	}
	
}

void deleteChar(){
	// Solo borrar si hay algo en el buffer
	if (bufferIndex > 0) {
		bufferIndex--;  // Quitar el último carácter del buffer
		
		// Borrar visualmente
		moveCursorLeft();
		uint64_t spaceX = CHAR_WIDTH*size;
		uint64_t spaceY = CHAR_HEIGHT*size;
		for(int y =0 ; y < spaceY; y++){
			for(int x =0; x < spaceX; x++){
				putPixel(DEFAULT_COLOR, cursorX + x, cursorY + y);
			}
		}
	}
}

void fillRectangle(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint64_t color) {
	uint64_t screen_w = VBE_mode_info->width;
	uint64_t screen_h = VBE_mode_info->height;
	if (width == 0 || height == 0 || x >= screen_w || y >= screen_h ) return; // nada que dibujar

	// Recortar ancho/alto para no salirnos de la pantalla
	uint64_t max_w = screen_w - x;
	uint64_t max_h = screen_h - y;
	if (width > max_w) width = max_w;
	if (height > max_h) height = max_h;

	for (uint64_t row = 0; row < height; ++row) {
		for (uint64_t col = 0; col < width; ++col) {
			putPixel(color, x + col, y + row);
		}
	}
}

void clearScreen(){
	fillRectangle(0, 0, VBE_mode_info->width, VBE_mode_info->height, DEFAULT_COLOR);
	cursorX = 0;
	cursorY = 0;
}

void scrollDown(){
	uint64_t lineHeight = CHAR_HEIGHT * size;
	uint8_t * framebuffer = (uint8_t *) (uint64_t) VBE_mode_info->framebuffer;
	for(uint64_t y = lineHeight; y < VBE_mode_info->height; y++){
		uint64_t dy= y - lineHeight;
		uint64_t srcOffset = y * VBE_mode_info->pitch;
		uint64_t destOffset = dy * VBE_mode_info->pitch;
		memcpy(framebuffer + destOffset, framebuffer + srcOffset, VBE_mode_info->pitch);
		}
		uint64_t clearStartY = VBE_mode_info->height - lineHeight;
		fillRectangle(0, clearStartY, VBE_mode_info->width, lineHeight, DEFAULT_COLOR);
}

void newLine(){
	// Guardar el \n en el buffer antes de hacer el salto
	if (bufferIndex <  SCREEN_BUFFER_SIZE) {
		screenBuffer[bufferIndex].ch = '\n';
		screenBuffer[bufferIndex].color = 0xFFFFFF; // Color no importa para \n
		bufferIndex++;
	}
	
	cursorX = 0;
	uint64_t spaceY = CHAR_HEIGHT*size;
	if(cursorY + spaceY < VBE_mode_info->height){
		cursorY += spaceY;
		fillRectangle(cursorX, cursorY, VBE_mode_info->width, spaceY, DEFAULT_COLOR); // limpiar línea
	}
	else{
		// Terminal llena: hacer scroll down
		scrollDown();
		cursorY = VBE_mode_info->height - spaceY;
	}

}

void moveCursorRight(){
	uint64_t spaceX = CHAR_WIDTH*size;
	if(cursorX + 2*spaceX <= VBE_mode_info->width){
		cursorX += spaceX;
	}
	else{
		// Antes de hacer el salto automático, guardar un \n en el buffer
		// para que al redibujar se respeten los saltos de línea
		if(bufferIndex < SCREEN_BUFFER_SIZE){
			screenBuffer[bufferIndex].ch = '\n';
			screenBuffer[bufferIndex].color = 0xFFFFFF; // Color por defecto
			screenBuffer[bufferIndex].size = size;
			bufferIndex++;
		}
		newLine();
	}
}

// Función para establecer la posición del cursor
void setCursorPosition(uint32_t x, uint32_t y){
	cursorX = x;
	cursorY = y;
}

void vPutChar(uint64_t c, uint64_t color){
	switch(c){
	case'\n':
		// Almacenar el salto de línea en el buffer con su tamaño
		if(bufferIndex < SCREEN_BUFFER_SIZE){
			screenBuffer[bufferIndex].ch = '\n';
			screenBuffer[bufferIndex].color = color;
			screenBuffer[bufferIndex].size = size;
			bufferIndex++;
		}
		newLine();
		break;
	case'\b':
	if(bufferIndex > 0){
		bufferIndex--;
	}
		deleteChar();
		break;
	default:
		// Almacenar el carácter en el buffer CON SU TAMAÑO
		if(bufferIndex < SCREEN_BUFFER_SIZE){
			screenBuffer[bufferIndex].ch = (char)c;
			screenBuffer[bufferIndex].color = color;
			screenBuffer[bufferIndex].size = size;
			bufferIndex++;
		}
		vd_drawChar(cursorX, cursorY, (char)c, color, size);
		moveCursorRight();
		break;
	}
}

void vprintString(const char* s, uint64_t color){
	for(const char* p = s; *p; ++p){
		vPutChar((uint64_t)(*p), color);
	}
}

void vd_drawChar(uint64_t x, uint64_t y, char ch, uint64_t color, uint64_t size){

	// font_bitmap contains glyphs for ASCII 32..126 (NCHARS = 95)
	if (ch >= 32 && (ch - 32) < NCHARS) {
		uint32_t idx = ch - 32;

		if (size == 0) size = 1; // asegurar tamaño mínimo

		for (uint32_t i = 0; i < CHAR_HEIGHT; i++) {
			uint8_t line = font_bitmap[idx][i];
			for (uint32_t j = 0; j < CHAR_WIDTH; j++) {
				if ((line << j) & 0x80) {
					// dibuja un bloque size x size por cada bit activo
					for (uint32_t dy = 0; dy < size; ++dy) {
						for (uint32_t dx = 0; dx < size; ++dx) {
							putPixel(color, x + j * size + dx, y + i * size + dy);
						}
					}
				}
			}
		}
	}
}

void vd_drawString(uint64_t x, uint64_t y, const char* s, uint64_t color, uint64_t size){
	if (!s) return; // proteger contra NULL
	if (size == 0) size = 1; // asegurar tamaño mínimo

	uint64_t step = CHAR_WIDTH * size;
	for (const char *p = s; *p; ++p) {
		vd_drawChar(x, y, *p, color, size);
		x += step;
	}
}

void vd_drawIntAt(int x, int y, int val, uint64_t color, uint64_t size, int right_align) {
	char buf[2];
	if (val == 0) {
		buf[0] = '0'; buf[1] = '\0';
		if (right_align) x -= CHAR_WIDTH * size;
		vd_drawString(x, y, buf, color, size);
		return;
	}
	char rev[16];
	int ri = 0;
	unsigned int v = (val < 0) ? (unsigned int)(-val) : (unsigned int)val;
	while (v > 0 && ri < (int)sizeof(rev)) {
		rev[ri++] = '0' + (v % 10);
		v /= 10;
	}
	int total_w = ri * CHAR_WIDTH * size;
	int px = right_align ? (x - total_w) : x;
	for (int i = ri - 1; i >= 0; --i) {
		buf[0] = rev[i]; buf[1] = '\0';
		vd_drawString(px, y, buf, color, size);
		px += CHAR_WIDTH * size;
	}
}

// Función para redibujar todo el contenido del buffer manteniendo los tamaños originales
void redrawScreen(void) {
	// Limpiar la pantalla
	clearScreen();
	
	// Resetear cursor al inicio
	cursorX = 0;
	cursorY = 0;
	
	// Calcular el espacio que ocupa un carácter con el NUEVO tamaño
	uint64_t charWidth = CHAR_WIDTH * size;
	uint64_t charHeight = CHAR_HEIGHT * size;
	uint64_t screenWidth = VBE_mode_info->width;
	uint64_t screenHeight = VBE_mode_info->height;
	
	// Redibujar todos los caracteres del buffer con el NUEVO tamaño
	for(int i = 0; i < bufferIndex; i++){
		char ch = screenBuffer[i].ch;
		uint32_t color = screenBuffer[i].color;
		
		if(ch == '\n'){
			// Salto de línea explícito
			cursorX = 0;
			cursorY += charHeight;
			
			// Si nos pasamos del límite vertical, hacer scroll
			if(cursorY + charHeight > screenHeight){
				scrollDown();
				cursorY = screenHeight - charHeight;
			}
		} else {
			// Verificar si el carácter cabe en la línea actual
			// Si no cabe, saltar línea ANTES de dibujarlo
			if(cursorX + charWidth > screenWidth){
				cursorX = 0;
				cursorY += charHeight;
				
				// Si nos pasamos del límite vertical, hacer scroll
				if(cursorY + charHeight > screenHeight){
					scrollDown();
					cursorY = screenHeight - charHeight;
				}
			}
			
			// Ahora sí, dibujar el carácter con el nuevo tamaño
			vd_drawChar(cursorX, cursorY, ch, color, size);
			
			// Mover cursor a la derecha
			cursorX += charWidth;
		}
	}

}

// Aumenta el tamaño de la fuente (max 4)
void increaseFontSize(void) {
	const uint8_t MAX_SIZE = 2;
	if (size < MAX_SIZE) {
		size++;
		redrawScreen();
	}
}

// Reduce el tamaño de la fuente (min 1)
void decreaseFontSize(void) {
	const uint8_t MIN_SIZE = 1;
	if (size > MIN_SIZE) {
		size--;
		redrawScreen();
	}
}

uint32_t getScreenWidth(void) {
	return VBE_mode_info->width;
}

uint32_t getScreenHeight(void) {
	return VBE_mode_info->height;
}



