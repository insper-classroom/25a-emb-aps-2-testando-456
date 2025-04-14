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

const int gpx = 26;
const int gpy = 27;

QueueHandle_t xQueueEventos;
QueueHandle_t xQueueBtn;

typedef struct {
    uint8_t tipo;   // 0=joystick1, 1=joystick2, 2=botao
    uint8_t eixo;   // 0=X, 1=Y (joystick); 0 (botao)
    int16_t valor;  // Joystick: -255 a 255; Botao: 10-15 (pressionado), 0 (solto)
} evento_t;

typedef struct {
    uint8_t id;     // GPIO do botao (10-15)
    uint8_t value;  // 1=pressionado, 0=solto
} evento_btn;

void btn_setup() {
    gpio_init(BTN_LM); gpio_set_dir(BTN_LM, GPIO_IN); gpio_pull_up(BTN_LM);
    gpio_init(BTN_RM); gpio_set_dir(BTN_RM, GPIO_IN); gpio_pull_up(BTN_RM);
    gpio_init(BTN_JUMP); gpio_set_dir(BTN_JUMP, GPIO_IN); gpio_pull_up(BTN_JUMP);
    gpio_init(BTN_RUN); gpio_set_dir(BTN_RUN, GPIO_IN); gpio_pull_up(BTN_RUN);
    gpio_init(BTN_SCROLL); gpio_set_dir(BTN_SCROLL, GPIO_IN); gpio_pull_up(BTN_SCROLL);
    gpio_init(BTN_INVENTORY); gpio_set_dir(BTN_INVENTORY, GPIO_IN); gpio_pull_up(BTN_INVENTORY);
}

void btn_callback(uint gpio, uint32_t events) {
    static uint32_t last_time = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_time < 50) return; // Debounce 50ms
    last_time = now;

    evento_btn infobtn;
    infobtn.id = gpio;
    infobtn.value = (events & GPIO_IRQ_EDGE_FALL) ? 1 : 0;

    xQueueSendFromISR(xQueueBtn, &infobtn, NULL);
}

static int calcula_janela(int new_value, int *buffer, int *index, int *sum) {
    *sum -= buffer[*index];
    buffer[*index] = new_value;
    *sum += new_value;
    *index = (*index + 1) % 5;
    return *sum / 5;
}

void joystick1_x_task(void *p) {
    int x_buffer[5] = {2047, 2047, 2047, 2047, 2047};
    int x_index = 0;
    int x_sum = 2047 * 5;
    evento_t evento = { .tipo = 0, .eixo = 0, .valor = 0 };
    adc_gpio_init(gpx);

    while (1) {
        adc_select_input(0);
        uint16_t raw_value = adc_read();
        int filtered_value = calcula_janela(raw_value, x_buffer, &x_index, &x_sum);
        int adjusted_value = (filtered_value - 2047) / 8;

        // Zona morta aumentada
        if (abs(adjusted_value) < 50) {
            adjusted_value = 0;
        } else {
            if (adjusted_value > 255) adjusted_value = 255;
            if (adjusted_value < -255) adjusted_value = -255;
        }

        // Envia evento se valor mudou
        if (adjusted_value != evento.valor) {
            evento.valor = adjusted_value;
            xQueueSend(xQueueEventos, &evento, pdMS_TO_TICKS(50));
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void joystick1_y_task(void *p) {
    int y_buffer[5] = {2047, 2047, 2047, 2047, 2047};
    int y_index = 0;
    int y_sum = 2047 * 5;
    evento_t evento = { .tipo = 0, .eixo = 1, .valor = 0 };
    adc_gpio_init(gpy);

    while (1) {
        adc_select_input(1);
        uint16_t raw_value = adc_read();
        int filtered_value = calcula_janela(raw_value, y_buffer, &y_index, &y_sum);
        int adjusted_value = (filtered_value - 2047) / 8;

        // Zona morta aumentada
        if (abs(adjusted_value) < 50) {
            adjusted_value = 0;
        } else {
            if (adjusted_value > 255) adjusted_value = 255;
            if (adjusted_value < -255) adjusted_value = -255;
        }

        // Envia evento se valor mudou
        if (adjusted_value != evento.valor) {
            evento.valor = adjusted_value;
            xQueueSend(xQueueEventos, &evento, pdMS_TO_TICKS(50));
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void btn_task(void *p) {
    evento_btn dataBTN;
    evento_t dataUart = { .tipo = 2, .eixo = 0, .valor = 0 };

    while (true) {
        if (xQueueReceive(xQueueBtn, &dataBTN, pdMS_TO_TICKS(100))) {
            dataUart.valor = (dataBTN.value == 1) ? dataBTN.id : 0;
            xQueueSend(xQueueEventos, &dataUart, pdMS_TO_TICKS(100));
        }
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
    xQueueBtn = xQueueCreate(50, sizeof(evento_btn));

    xTaskCreate(joystick1_x_task, "J1_X", 4096, NULL, 1, NULL);
    xTaskCreate(joystick1_y_task, "J1_Y", 4096, NULL, 1, NULL);
    xTaskCreate(btn_task, "BTN", 4096, NULL, 1, NULL);
    xTaskCreate(uart_task, "UART", 4096, NULL, 2, NULL);

    vTaskStartScheduler();
    while (true);
}