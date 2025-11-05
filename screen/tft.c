#include "tft.h"
#include "../buttons/pins.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>
#include "ili9341/ili9341.h"
#include "gfx/gfx.h"
#include "gfx/font.h" 


static void tft_draw_string(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size) {
    int16_t cursor_x = x;
    int16_t cursor_y = y;

    while (*str) {
        if (*str == '\n') {
            cursor_y += 5 * size; 
            cursor_x = x;
        } else {
            GFX_drawChar(cursor_x, cursor_y, *str, color, bg, size, size);
            cursor_x += 6 * size; 
        }
        str++;
    }
}


void tft_init(void) {
    LCD_setSPIperiph(SPI_PORT);
   
    LCD_setPins(LCD_DC_PIN, LCD_CS_PIN, LCD_RESET_PIN, LCD_SCK_PIN, LCD_TX_PIN);
    
    LCD_initDisplay();
    
    LCD_setRotation(1); 
    
    GFX_fillScreen(ILI9341_WHITE);

   GFX_createFramebuf();

    GFX_fillScreen(ILI9341_WHITE);
    
    tft_draw_string(10, 10, "MARCHA", ILI9341_BLACK, ILI9341_WHITE, 3);

    GFX_flush();
}

void tft_update_gear_string(const char *gear_str) {
    
    // CORREÇÃO (Flicker): Limpa apenas o retângulo
    GFX_fillRect(120, 80, 100, 50, ILI9341_WHITE);

    // CORREÇÃO (Lógica): Não precisa mais de 'sprintf' ou 'if'.
    // A string "N" ou "1" já vem pronta do Python.
    tft_draw_string(140, 80, gear_str, ILI9341_RED, ILI9341_WHITE, 8);
}

