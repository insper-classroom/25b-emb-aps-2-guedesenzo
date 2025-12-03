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
volatile uint32_t last_time = 0;

//encoder
volatile uint offsetA,offsetB, smA, smB, offsetC,offsetD, smC, smD; 

/* Queues */    
QueueHandle_t XqueueCmd, xQueueDisplay;

//abstrai o init do encoder
void get_encoder_counts(uint sm_a, uint sm_b, int32_t *count1, int32_t *count2) {
    
    pio_sm_exec_wait_blocking(encoderPIO, sm_a, pio_encode_in(pio_x, 32));
    pio_sm_exec_wait_blocking(encoderPIO, sm_b, pio_encode_in(pio_x, 32));
    
    *count1 = pio_sm_get_blocking(encoderPIO, sm_a);
    *count2 = pio_sm_get_blocking(encoderPIO, sm_b);

}


void btn_callback(uint gpio, uint32_t events){

    uint32_t current_time = to_us_since_boot(get_absolute_time());

    // debounce de software: ignora eventos ocorridos em menos de 20ms (20000us)
    if (current_time - last_time < 20000) {
        return; 
    }
    last_time = current_time;
    
    button_t btn_msg;
    // na hora de testar, verificar se funciona mesmo sem definir o valor inicial do botao.
    
    // Fall Edge (Apertar o botão)
    if(events == 0x4){ 
        btn_msg.val = 1;
        
        if (gpio == GPIO_BTN_RESET)      btn_msg.key = 1;
        else if (gpio == GPIO_BTN_HORN)       btn_msg.key = 2;
        else if (gpio == GPIO_BTN_HAZARDS)    btn_msg.key = 4; // BOTAO COM PROBLEMA, NAO ENVIA ISSO CORRETAMENTE, JA TESTEI COM OUTROS BOTOES, EH ERRO FISICO!!!
        else if (gpio == GPIO_BTN_FAROL)      btn_msg.key = 5;
        else if (gpio == GPIO_BTN_FAROL_ALTO) btn_msg.key = 6; 
        else if (gpio == GPIO_BTN_IGNICAO)    btn_msg.key = 7;  
        else if (gpio == GPIO_BTN_TWO_STEP)   btn_msg.key = 8;
        else if (gpio == GPIO_BTN_NITRO)      btn_msg.key = 9; 


        xQueueSendFromISR(XqueueCmd, &btn_msg, 0);
    }
    
    // Rise Edge (Soltar o botão)
    else if(events == 0x8){ 
        btn_msg.val = 0;

        if (gpio == GPIO_BTN_HORN)            btn_msg.key = 2;
        else if (gpio == GPIO_BTN_HAZARDS)    btn_msg.key = 4;
        else if (gpio == GPIO_BTN_FAROL)      btn_msg.key = 5;
        else if (gpio == GPIO_BTN_FAROL_ALTO) btn_msg.key = 6;
        else if (gpio == GPIO_BTN_IGNICAO)    btn_msg.key = 7; 
        else if (gpio == GPIO_BTN_TWO_STEP)   btn_msg.key = 8;
        else if (gpio == GPIO_BTN_NITRO)      btn_msg.key = 9; 

        xQueueSendFromISR(XqueueCmd, &btn_msg, 0);


    }  
}

void init_buttons(){

    // botoes
    gpio_init(GPIO_BTN_RESET);
    gpio_set_dir(GPIO_BTN_RESET, GPIO_IN);
    gpio_pull_up(GPIO_BTN_RESET);
    gpio_set_irq_enabled_with_callback(GPIO_BTN_RESET, GPIO_IRQ_EDGE_FALL, true, &btn_callback); // primeria vez. definindo o callback

    gpio_init(GPIO_BTN_HORN);
    gpio_set_dir(GPIO_BTN_HORN, GPIO_IN);
    gpio_pull_up(GPIO_BTN_HORN);
    gpio_set_irq_enabled(GPIO_BTN_HORN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true); // extras

    gpio_init(GPIO_BTN_HAZARDS);
    gpio_set_dir(GPIO_BTN_HAZARDS, GPIO_IN);
    gpio_pull_up(GPIO_BTN_HAZARDS);
    gpio_set_irq_enabled(GPIO_BTN_HAZARDS, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    gpio_init(GPIO_BTN_FAROL);
    gpio_set_dir(GPIO_BTN_FAROL, GPIO_IN);
    gpio_pull_up(GPIO_BTN_FAROL);
    gpio_set_irq_enabled(GPIO_BTN_FAROL, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    gpio_init(GPIO_BTN_FAROL_ALTO);
    gpio_set_dir(GPIO_BTN_FAROL_ALTO, GPIO_IN);
    gpio_pull_up(GPIO_BTN_FAROL_ALTO);
    gpio_set_irq_enabled(GPIO_BTN_FAROL_ALTO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    gpio_init(GPIO_BTN_IGNICAO);
    gpio_set_dir(GPIO_BTN_IGNICAO, GPIO_IN);
    gpio_pull_up(GPIO_BTN_IGNICAO);
    gpio_set_irq_enabled(GPIO_BTN_IGNICAO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    gpio_init(GPIO_BTN_TWO_STEP);
    gpio_set_dir(GPIO_BTN_TWO_STEP, GPIO_IN);
    gpio_pull_up(GPIO_BTN_TWO_STEP);
    gpio_set_irq_enabled(GPIO_BTN_TWO_STEP, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    
    gpio_init(GPIO_BTN_NITRO);
    gpio_set_dir(GPIO_BTN_NITRO, GPIO_IN);
    gpio_pull_up(GPIO_BTN_NITRO);
    gpio_set_irq_enabled(GPIO_BTN_NITRO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);


    // encoder 1
    offsetA = pio_add_program(encoderPIO, &quadratureA_program);
    offsetB = pio_add_program(encoderPIO, &quadratureB_program);
    smA = pio_claim_unused_sm(encoderPIO, true);
    smB = pio_claim_unused_sm(encoderPIO, true);
    quadratureA_program_init(encoderPIO, smA, offsetA, QUADRATURE_A_PIN, QUADRATURE_B_PIN);
    quadratureB_program_init(encoderPIO, smB, offsetB, QUADRATURE_A_PIN, QUADRATURE_B_PIN);
    
    // encoder 2
    offsetC = pio_add_program(encoderPIO, &quadratureA_program);
    offsetD = pio_add_program(encoderPIO, &quadratureB_program); 
    smC = pio_claim_unused_sm(encoderPIO, true);
    smD = pio_claim_unused_sm(encoderPIO, true);
    quadratureA_program_init(encoderPIO, smC, offsetC, QUADRATURE_C_PIN, QUADRATURE_D_PIN); 
    quadratureB_program_init(encoderPIO, smD, offsetD, QUADRATURE_C_PIN, QUADRATURE_D_PIN); 
}

void encoder_task(void *p){
    int32_t last_position_1 = 0;
    int32_t last_position_2 = 0;
    button_t encoder_msg;
    int32_t countA,countB, countC, countD;

    while(1) {
        
        vTaskDelay(pdMS_TO_TICKS(100)); 
        
        // encoder 1
        get_encoder_counts(smA, smB, &countA, &countB);
        printf("countA:%d\n", countA);
        printf("countB:%d\n", countB);

        int32_t current_position_1 = (countA + countB)/2;
        int32_t delta_1 = current_position_1 - last_position_1;
        
        if(delta_1 != 0) {
            encoder_msg.key = 3;
            encoder_msg.val = delta_1;
            xQueueSend(XqueueCmd, &encoder_msg, 0); 
        }
        last_position_1 = current_position_1;

        
        // encoder 2
        get_encoder_counts(smC, smD, &countC, &countD);
        
        int32_t current_position_2 = (countC + countD)/2;
        int32_t delta_2 = current_position_2 - last_position_2;
        
        if(delta_2 != 0) {
            encoder_msg.key = 10;
            encoder_msg.val = delta_2;
            xQueueSend(XqueueCmd, &encoder_msg, 0); 
        }
        last_position_2 = current_position_2;
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

       if(xQueueReceiveFromISR(XqueueCmd, &button, 0)){
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


int main(void)
{
    stdio_init_all();
    
    XqueueCmd = xQueueCreate(4, sizeof(button_t));
    xQueueDisplay = xQueueCreate(10, sizeof(char[4]));   
    
    init_buttons();
    xTaskCreate(uart_task, "Task 1", 1024, NULL, 1, NULL);
    xTaskCreate(encoder_task, "Task 2", 1024, NULL, 1, NULL);
    xTaskCreate(display_task, "Task 3", 2048, NULL, 1, NULL);
    
    vTaskStartScheduler();

    // Should never reach here
    for (;;);
}

