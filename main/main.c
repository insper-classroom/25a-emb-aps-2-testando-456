#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <hardware/adc.h>
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>

const int BTN_LM = 15;
const int BTN_RM = 14;
const int BTN_SCROLL = 13;
const int BTN_INVENTORY = 12;
const int BTN_JUMP = 11;
const int BTN_RUN = 10;

// const int gpx = 28;  // Joystick 1 X (ADC2)
// const int gpy = 29;  // Joystick 1 Y (ADC3)
const int gpx2 = 26; // Joystick 2 X (ADC0)
const int gpy2 = 27; // Joystick 2 Y (ADC1)

QueueHandle_t xQueueEventos;

typedef struct {
    uint8_t tipo;   // 0=joystick1, 1=joystick2, 2=botao
    uint8_t eixo;   // 0=X, 1=Y (joystick); 0 (botao)
    int16_t valor;  // Joystick: -255 a 255; Botao: 10-15 (pressionado), 0 (solto)
} evento_t;

void btn_setup() {
    gpio_init(BTN_LM); gpio_set_dir(BTN_LM, GPIO_IN); gpio_pull_up(BTN_LM);
    gpio_init(BTN_RM); gpio_set_dir(BTN_RM, GPIO_IN); gpio_pull_up(BTN_RM);
    gpio_init(BTN_JUMP); gpio_set_dir(BTN_JUMP, GPIO_IN); gpio_pull_up(BTN_JUMP);
    gpio_init(BTN_RUN); gpio_set_dir(BTN_RUN, GPIO_IN); gpio_pull_up(BTN_RUN);
    gpio_init(BTN_SCROLL); gpio_set_dir(BTN_SCROLL, GPIO_IN); gpio_pull_up(BTN_SCROLL);
    gpio_init(BTN_INVENTORY); gpio_set_dir(BTN_INVENTORY, GPIO_IN); gpio_pull_up(BTN_INVENTORY);
}

void btn_callback(uint gpio, uint32_t events) {
    evento_t evento;
    evento.tipo = 2;
    evento.eixo = 0;
    evento.valor = (events & GPIO_IRQ_EDGE_FALL) ? gpio : 0;
    xQueueSendFromISR(xQueueEventos, &evento, NULL);
}

static int calcula_janela(int new_value, int *buffer, int *index, int *sum) {
    *sum -= buffer[*index];
    buffer[*index] = new_value;
    *sum += new_value;
    *index = (*index + 1) % 5;
    return *sum / 5;
}

// void joystick1_x_task(void *p) {
//     int x_buffer[5] = {2047, 2047, 2047, 2047, 2047};
//     int x_index = 0;
//     int x_sum = 2047 * 5;
//     evento_t evento = { .tipo = 0, .eixo = 0, .valor = 0 };
//     adc_gpio_init(gpx);

//     while (1) {
//         adc_select_input(2); // ADC2 para GP28
//         uint16_t raw_value = adc_read();
//         int filtered_value = calcula_janela(raw_value, x_buffer, &x_index, &x_sum);
//         int adjusted_value = (filtered_value - 2047) / 8;

//         if (abs(adjusted_value) < 50) {
//             adjusted_value = 0;
//         } else {
//             if (adjusted_value > 255) adjusted_value = 255;
//             if (adjusted_value < -255) adjusted_value = -255;
//         }

//         if (adjusted_value != evento.valor) {
//             evento.valor = adjusted_value;
//             xQueueSend(xQueueEventos, &evento, pdMS_TO_TICKS(50));
//         }

//         vTaskDelay(pdMS_TO_TICKS(100));
//     }
// }

// void joystick1_y_task(void *p) {
//     int y_buffer[5] = {2047, 2047, 2047, 2047, 2047};
//     int y_index = 0;
//     int y_sum = 2047 * 5;
//     evento_t evento = { .tipo = 0, .eixo = 1, .valor = 0 };
//     adc_gpio_init(gpy);

//     while (1) {
//         adc_select_input(3); // ADC3 para GP29
//         uint16_t raw_value = adc_read();
//         int filtered_value = calcula_janela(raw_value, y_buffer, &y_index, &y_sum);
//         int adjusted_value = (filtered_value - 2047) / 8;

//         if (abs(adjusted_value) < 50) {
//             adjusted_value = 0;
//         } else {
//             if (adjusted_value > 255) adjusted_value = 255;
//             if (adjusted_value < -255) adjusted_value = -255;
//         }

//         if (adjusted_value != evento.valor) {
//             evento.valor = adjusted_value;
//             xQueueSend(xQueueEventos, &evento, pdMS_TO_TICKS(50));
//         }

//         vTaskDelay(pdMS_TO_TICKS(100));
//     }
// }

void joystick2_x_task(void *p) {
    int x_buffer[5] = {2047, 2047, 2047, 2047, 2047};
    int x_index = 0;
    int x_sum = 2047 * 5;
    evento_t evento = { .tipo = 1, .eixo = 0, .valor = 0 };
    adc_gpio_init(gpx2);

    while (1) {
        adc_select_input(0); // ADC0 para GP26
        uint16_t raw_value = adc_read();
        int filtered_value = calcula_janela(raw_value, x_buffer, &x_index, &x_sum);
        int adjusted_value = (filtered_value - 2047) / 8;

        if (abs(adjusted_value) < 50) {
            adjusted_value = 0;
        } else {
            if (adjusted_value > 255) adjusted_value = 255;
            if (adjusted_value < -255) adjusted_value = -255;
        }

        if (adjusted_value != evento.valor) {
            evento.valor = adjusted_value;
            xQueueSend(xQueueEventos, &evento, pdMS_TO_TICKS(50));
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void joystick2_y_task(void *p) {
    int y_buffer[5] = {2047, 2047, 2047, 2047, 2047};
    int y_index = 0;
    int y_sum = 2047 * 5;
    evento_t evento = { .tipo = 1, .eixo = 1, .valor = 0 };
    adc_gpio_init(gpy2);

    while (1) {
        adc_select_input(1); // ADC1 para GP27
        uint16_t raw_value = adc_read();
        int filtered_value = calcula_janela(raw_value, y_buffer, &y_index, &y_sum);
        int adjusted_value = (filtered_value - 2047) / 8;

        if (abs(adjusted_value) < 50) {
            adjusted_value = 0;
        } else {
            if (adjusted_value > 255) adjusted_value = 255;
            if (adjusted_value < -255) adjusted_value = -255;
        }

        if (adjusted_value != evento.valor) {
            evento.valor = adjusted_value;
            xQueueSend(xQueueEventos, &evento, pdMS_TO_TICKS(50));
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void uart_task(void *p) {
    evento_t evento;
    while (1) {
        if (xQueueReceive(xQueueEventos, &evento, pdMS_TO_TICKS(50))) {
            putchar_raw(evento.tipo);
            putchar_raw(evento.eixo);
            putchar_raw(evento.valor & 0xFF);
            putchar_raw((evento.valor >> 8) & 0xFF);
            putchar_raw(0xFF);
        }
    }
}

int main() {
    stdio_init_all();
    adc_init();
    btn_setup();
    gpio_set_irq_enabled_with_callback(BTN_LM, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);
    gpio_set_irq_enabled_with_callback(BTN_RM, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);
    gpio_set_irq_enabled_with_callback(BTN_SCROLL, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);
    gpio_set_irq_enabled_with_callback(BTN_INVENTORY, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);
    gpio_set_irq_enabled_with_callback(BTN_JUMP, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);
    gpio_set_irq_enabled_with_callback(BTN_RUN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);

    xQueueEventos = xQueueCreate(50, sizeof(evento_t));

    // xTaskCreate(joystick1_x_task, "J1_X", 4096, NULL, 1, NULL); --> funcionando ja
    // xTaskCreate(joystick1_y_task, "J1_Y", 4096, NULL, 1, NULL);
    xTaskCreate(joystick2_x_task, "J2_X", 4096, NULL, 1, NULL);
    xTaskCreate(joystick2_y_task, "J2_Y", 4096, NULL, 1, NULL);
    xTaskCreate(uart_task, "UART", 4096, NULL, 2, NULL);

    vTaskStartScheduler();
    while (true);
}