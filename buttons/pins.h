#ifndef PINS_H
#define PINS_H

// botoes
#define GPIO_BTN_RESET 4 
#define GPIO_BTN_HORN 5 
#define GPIO_BTN_HAZARDS 2 
#define GPIO_BTN_FAROL 3 
#define GPIO_BTN_FAROL_ALTO 6 
#define GPIO_BTN_IGNICAO 7 
#define GPIO_BTN_TWO_STEP 8
#define GPIO_BTN_NITRO 9 

// encoder
#define QUADRATURE_A_PIN 10
#define QUADRATURE_B_PIN 11
#define QUADRATURE_C_PIN 12 
#define QUADRATURE_D_PIN 13 

// tela
#define LCD_LITE_PIN  15
#define LCD_RESET_PIN 16
#define LCD_CS_PIN    17
#define LCD_SCK_PIN   18
#define LCD_TX_PIN    19
#define LCD_DC_PIN    22
#define SPI_PORT      spi0
#define SPI_RX_PIN    -1

#endif