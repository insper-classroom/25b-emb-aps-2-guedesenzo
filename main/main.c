#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include "../buttons/button.h"
#include "../buttons/pins.h"
#include "hardware/pio.h"
#include "../encoder/pico_emb.pio.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "../screen/tft.h"
#include "../screen/gfx/gfx.h"

#define UART_TX_PIN 0
#define UART_RX_PIN 1

// definindo as variaveis globais:
button_t button1, button2;
volatile PIO encoderPIO = pio0;
volatile uint offsetA,offsetB, smA, smB; 

/* Queues */    
QueueHandle_t XqueueCmd, xQueueDisplay;



void btn_callback(uint gpio, uint32_t events){

    if(events == 0x4){ // fall edge 0 -> 1 
        
        if (gpio == GPIO_BLUE_BUTTON){ // caso de aperto e soltar
            
            button1.key = 1;
            button1.val = !button1.val;

            if(xQueueSendFromISR(XqueueCmd,&button1,0)){
                // printf("azul enviado com sucesso: val: %d, key: %d\n", button1.val, button1.key);
            }else{
                // printf("falha ao enviar fila\n");
            }

        }
        else if(gpio == GPIO_BLACK_BUTTON){ // caso de aperto e segurar
            button2.key = 2;
            button2.val = 1;

            if (xQueueSendFromISR(XqueueCmd,&button2,0)){
                // printf("preto enviado com sucesso: val:%d, key:%d\n", button2.val, button2.key);
            }else{
                // printf("falha ao enviar fila\n");
            }

        }
        
    }

    else{ // rise edge eh 1 -> 0
       
        if(gpio == GPIO_BLACK_BUTTON){
            button2.key = 2;
            button2.val = 0;
            if (xQueueSendFromISR(XqueueCmd,&button2,0)){
                // printf("preto enviado com sucesso: val:%d, key:%d\n", button2.val, button2.key);
            }else{
                // printf("falha ao enviar fila\n");
            }

        }
    }  
}

void encoder_task(void *p){
    int32_t last_position = 0;
    button_t encoder;
    encoder.key = 3;

    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(300)); 
        
        //push 
        pio_sm_exec_wait_blocking(encoderPIO, smA, pio_encode_in(pio_x, 32));
        pio_sm_exec_wait_blocking(encoderPIO, smB, pio_encode_in(pio_x, 32));
        
        //pull
        int32_t countA = pio_sm_get_blocking(encoderPIO, smA);
        // printf("%d\n",countA);
        int32_t countB = pio_sm_get_blocking(encoderPIO, smB);
        // printf("%d\n",countB);

        int32_t current_position = (countA + countB)/2; // para pegar o valor de cada girada/2
        int32_t delta = current_position - last_position;
        
        if(delta != 0) {
            // printf("Delta: %d | Posicao: %d (A:%d B:%d)\n",delta, current_position, countA, countB);
            encoder.val = delta;

            if (xQueueSendFromISR(XqueueCmd,&encoder,0)){
                // printf("preto enviado com sucesso: val:%d, key:%d\n", button2.val, button2.key);
            }else{
                // printf("falha ao enviar fila\n");
            }
        }
        
        last_position = current_position;
    }

}

// por hora incompleta
void uart_task(void *p){
    button_t button;
    char rx_gear_string[4]; //buffer
    int rx_char_index = 0;
    char c;

    uart_init(uart0, 115200);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // printf("entrei uart");
    while(1){

       if(xQueueReceive(XqueueCmd, &button, portMAX_DELAY)){
            // printf("value: %d key:%d\n", button.val,button.key);

            // byte representando tecla
            uart_putc_raw(uart0, button.key);

            // // LSB
            uart_putc_raw(uart0, button.val);

            // // MSB
            uart_putc_raw(uart0, button.val >> 8);

            // // byte de sincronismo
            uart_putc_raw(uart0, 0xFF);

            // delay
            vTaskDelay(pdMS_TO_TICKS(100));

            // stop byte
            // uart_putc_raw(uart0, -1);
       }

       // para verificar se esta chegando dados da marcha
       if (uart_is_readable(uart0)) {
           c = uart_getc(uart0);
           
           // se for o fim da linha (enviado pelo Python),
           // finalize a string e envie para a fila.
           if (c == '\n') {
               rx_gear_string[rx_char_index] = '\0'; // adiciona o \0 e explicita que a string acabou
               
               // envia a string completa para a fila do display (caractere + \0)
               if (rx_char_index > 0) { // so envia se não for uma string vazia
                   xQueueSend(xQueueDisplay, &rx_gear_string, 0);
               }
               rx_char_index = 0; // reseta o buffer
           
           // se for um caractere normal, adiciona ao buffer
           } else if (rx_char_index < (sizeof(rx_gear_string) -1)) {
               rx_gear_string[rx_char_index] = c;
               rx_char_index++;
           }
             
        }
    }
}

void init_buttons(){

    // init das variaveis do botao
    gpio_init(GPIO_BLUE_BUTTON);
    gpio_init(GPIO_BLACK_BUTTON);

    gpio_set_dir(GPIO_BLUE_BUTTON, GPIO_IN);
    gpio_set_dir(GPIO_BLACK_BUTTON, GPIO_IN);

    gpio_pull_up(GPIO_BLUE_BUTTON); // apenas para mostrar que nao vai.
    gpio_pull_up(GPIO_BLACK_BUTTON);

    gpio_set_irq_enabled_with_callback(GPIO_BLUE_BUTTON, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);

    gpio_set_irq_enabled(GPIO_BLACK_BUTTON, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);


    // init nas variaveis do encoder.  
    offsetA = pio_add_program(encoderPIO, &quadratureA_program);
    
    offsetB = pio_add_program(encoderPIO, &quadratureB_program);
    
    smA = pio_claim_unused_sm(encoderPIO, true);
    
    smB = pio_claim_unused_sm(encoderPIO, true);
    
    quadratureA_program_init(encoderPIO, smA, offsetA, QUADRATURE_A_PIN, QUADRATURE_B_PIN);
    quadratureB_program_init(encoderPIO, smB, offsetB, QUADRATURE_A_PIN, QUADRATURE_B_PIN);
   
}


void display_task(void *p) {
    tft_init();
    char received_code[4];

    while (1) {
       
        if (xQueueReceive(xQueueDisplay, &received_code, portMAX_DELAY)) {
            tft_update_gear_string(received_code);
            GFX_flush();
        }
    }
}


void display_task_teste(void *p) {

    tft_init();
    char *marchas_teste[] = {"N", "1", "2", "3", "4", "5", "R"};
    int marcha_atual_idx = 0;
    int num_marchas = 7; 

    while (1) {

        const char *marcha_para_exibir = marchas_teste[marcha_atual_idx];

        
        tft_update_gear_string(marcha_para_exibir);

        
        GFX_flush();

        
        marcha_atual_idx = (marcha_atual_idx + 1) % num_marchas;

        
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
  
}


int main(void)
{
    stdio_init_all();

    button1.val = 0;
    button2.val = 0;
    
    // XqueueCmd = xQueueCreate(250, sizeof(button_t));
    // xQueueDisplay = xQueueCreate(10, sizeof(char[4]));   
    
    init_buttons();
    // xTaskCreate(uart_task, "Task 1", 1024, NULL, 1, NULL);
    // xTaskCreate(encoder_task, "Task 2", 1024, NULL, 1, NULL);
    // xTaskCreate(display_task, "Task 3", 2048, NULL, 1, NULL);
    xTaskCreate(display_task_teste, "Task_teste", 2048, NULL, 1, NULL);
    
    vTaskStartScheduler();

    // Should never reach here
    for (;;);
}

// falta nessa versao: 
// - Ajustar a fila
// - Acabar a uart
