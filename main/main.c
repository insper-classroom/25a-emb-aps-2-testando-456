/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "ssd1306.h"
#include "gfx.h"

#include "pico/stdlib.h"
#include <stdio.h>

const int BTN_RM = 15;
const int BTN_RM = 14;
const int BTN_SCROLL = 13;
const int BTN_INVENTORY = 12;
const int BTN_JUMP = 11;
const int BTN_RUN = 10;


QueueHandle_t xQueueComandos;



typedef struct {
    uint8_t tipo;   // 0 = joystick1
    uint8_t tecla;  // 0 = W, 1 = A, 2 = S, 3 = D
    uint8_t estado; // 1 = pressionado, 0 = solto
} evento_t;



void move_x_task(void *p) {



    while (true) {

        
        vTaskDelay(pdMS_TO_TICKS(100));
        printf("Rodando BTN_TASK");

    }
}


void move_y_task(void *p) {



    while (true) {

        
        vTaskDelay(pdMS_TO_TICKS(100));
        printf("Rodando BTN_TASK");

    }
}

void look_x_task(void *p) {



    while (true) {

        
        vTaskDelay(pdMS_TO_TICKS(100));
        printf("Rodando BTN_TASK");

    }
}

void look_y_task(void *p) {



    while (true) {

        
        vTaskDelay(pdMS_TO_TICKS(100));
        printf("Rodando BTN_TASK");

    }
}



void btn_task(void *p) {



    while (true) {

        
        vTaskDelay(pdMS_TO_TICKS(100));
        printf("Rodando BTN_TASK");

    }
}


void uart_task(void *p) {



    while (true) {

        
        vTaskDelay(pdMS_TO_TICKS(100));
        printf("Rodando UART_TASK");
    }
}


int main() {
    stdio_init_all();


    xQueue = xQueueCreate(32 , sizeof(evento_t));

    
    xTaskCreate(move_x_task, "Task de Mover em X", 4095, NULL, 1, NULL);
    xTaskCreate(move_y_task, "Task de Mover em Y", 4095, NULL, 1, NULL);
    xTaskCreate(look_x_task, "Task de Olhar em X", 4095, NULL, 1, NULL);
    xTaskCreate(look_y_task, "Task de Olhar em Y", 4095, NULL, 1, NULL);
    xTaskCreate(uart_task, "Task do UART", 4095, NULL, 1, NULL);     
    xTaskCreate(btn_task, "Task de Botões", 4095, NULL, 1, NULL);

    vTaskStartScheduler();


    while (true)
        ;
}
