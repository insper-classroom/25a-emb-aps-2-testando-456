#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <hardware/adc.h>
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>

const int BTN_W = 14;     // Tecla 'w' (andar para frente)
const int BTN_A = 15;     // Tecla 'a' (esquerda)
const int BTN_S = 20;     // Tecla 's' (trás)
const int BTN_D = 21;     // Tecla 'd' (direita)
const int BTN_MACRO = 22; // Botão para reproduzir macro

const int JOY_X = 26;     // Joystick X (ADC0, mouse)
const int JOY_Y = 27;     // Joystick Y (ADC1, mouse)

QueueHandle_t xQueueEventos;

typedef struct {
    uint8_t tipo;   // 1=joystick, 2=botao
    uint8_t eixo;   // 0=X, 1=Y (joystick); 0 (botao)
    int16_t valor;  // Joystick: -255 a 255; Botao: 14,15,20,21,22 (pressionado), 0 (solto)
} evento_t;

void btn_setup() {
    gpio_init(BTN_W); gpio_set_dir(BTN_W, GPIO_IN); gpio_pull_up(BTN_W);
    gpio_init(BTN_A); gpio_set_dir(BTN_A, GPIO_IN); gpio_pull_up(BTN_A);
    gpio_init(BTN_S); gpio_set_dir(BTN_S, GPIO_IN); gpio_pull_up(BTN_S);
    gpio_init(BTN_D); gpio_set_dir(BTN_D, GPIO_IN); gpio_pull_up(BTN_D);
    gpio_init(BTN_MACRO); gpio_set_dir(BTN_MACRO, GPIO_IN); gpio_pull_up(BTN_MACRO);
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

void joystick_x_task(void *p) {
    int x_buffer[5] = {2047, 2047, 2047, 2047, 2047};
    int x_index = 0;
    int x_sum = 2047 * 5;
    evento_t evento = { .tipo = 1, .eixo = 0, .valor = 0 };
    adc_gpio_init(JOY_X);

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

void joystick_y_task(void *p) {
    int y_buffer[5] = {2047, 2047, 2047, 2047, 2047};
    int y_index = 0;
    int y_sum = 2047 * 5;
    evento_t evento = { .tipo = 1, .eixo = 1, .valor = 0 };
    adc_gpio_init(JOY_Y);

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
    gpio_set_irq_enabled_with_callback(BTN_W, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);
    gpio_set_irq_enabled_with_callback(BTN_S, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);
    gpio_set_irq_enabled_with_callback(BTN_D, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);
    gpio_set_irq_enabled_with_callback(BTN_MACRO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);

    xQueueEventos = xQueueCreate(50, sizeof(evento_t));

    xTaskCreate(joystick_x_task, "JOY_X", 4096, NULL, 1, NULL);
    xTaskCreate(joystick_y_task, "JOY_Y", 4096, NULL, 1, NULL);
    xTaskCreate(uart_task, "UART", 4096, NULL, 2, NULL);

    vTaskStartScheduler();
    while (true);
}