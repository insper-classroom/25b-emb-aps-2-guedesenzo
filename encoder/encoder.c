

#include "pico_emb.pio.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define QUADRATURE_A_PIN 10
#define QUADRATURE_B_PIN 11

int main()
{
    stdio_init_all();

    PIO encoderPIO = pio0;

    uint offsetA = pio_add_program(encoderPIO, &quadratureA_program);
    uint smA = pio_claim_unused_sm(encoderPIO, true);

    uint offsetB = pio_add_program(encoderPIO, &quadratureB_program);
    uint smB = pio_claim_unused_sm(encoderPIO, true);

    quadratureA_program_init(encoderPIO, smA, offsetA, QUADRATURE_A_PIN, QUADRATURE_B_PIN);
    quadratureB_program_init(encoderPIO, smB, offsetB, QUADRATURE_A_PIN, QUADRATURE_B_PIN);

    while (true)
    {
        sleep_ms(1000);

        pio_sm_exec_wait_blocking(encoderPIO, smA, pio_encode_in(pio_x, 32));
        pio_sm_exec_wait_blocking(encoderPIO, smB, pio_encode_in(pio_x, 32));

        int32_t countA = pio_sm_get_blocking(encoderPIO, smA);
        printf("countA %d\n", countA);
        
        int32_t countB = pio_sm_get_blocking(encoderPIO, smB);
        printf("countB %d\n", countB);

        
    }
}









// teste: 

/*
// void encoder_task(void *p){
//     printf("Encoder task iniciada\n");
//     printf("Pino A: %d, Pino B: %d\n", QUADRATURE_A_PIN, QUADRATURE_B_PIN);
    
//     int count = 0;

//     bool last_a = gpio_get(QUADRATURE_A_PIN);
//     bool last_b = gpio_get(QUADRATURE_B_PIN);
    
//     while (true) {
//         bool a_state = gpio_get(QUADRATURE_A_PIN);
//         bool b_state = gpio_get(QUADRATURE_B_PIN);

//         // Força print a cada 20 leituras para ver se está lendo
//         if(a_state != last_a || b_state != last_b) {
//             printf("MUDOU! A: %d->%d, B: %d->%d\n", last_a, a_state, last_b, b_state);
//             last_a = a_state;
//             last_b = b_state;
//         }


//         vTaskDelay(pdMS_TO_TICKS(10));
//     }
// }


init para teste : 

  // define os botoes e coloca eles 
    
    // para teste
    // gpio_init(QUADRATURE_A_PIN);
    // gpio_set_dir(QUADRATURE_A_PIN, GPIO_IN);
    // gpio_pull_up(QUADRATURE_A_PIN);
    
    // gpio_init(QUADRATURE_B_PIN);
    // gpio_set_dir(QUADRATURE_B_PIN, GPIO_IN);
    // gpio_pull_up(QUADRATURE_B_PIN);


// Delete all PIO setup code (offsets, sms, quadratureA_program_init, etc.)
*/