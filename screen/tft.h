#ifndef TFT_H
#define TFT_H

// Prepara o hardware do display (SPI, etc.) e limpa a tela.
void tft_init(void);

// Desenha um número grande no meio da tela.
// A função vai apagar o número antigo antes de desenhar o novo.
void tft_update_gear_string(const char *gear_str);

// Função de teste para desenhar o "3" imediatamente ao ligar.
void tft_draw_test_digit(void);

#endif