#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

QueueHandle_t xQueueBtn;
SemaphoreHandle_t xSemaphore_r;
SemaphoreHandle_t xSemaphore_y;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) {
        int pin = (int)gpio;
        xQueueSendFromISR(xQueueBtn, &pin, 0);
    }
}

void btn_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    int pin;
    while (true) {
        if (xQueueReceive(xQueueBtn, &pin, portMAX_DELAY) == pdTRUE) {
            if (pin == BTN_PIN_R) {
                xSemaphoreGive(xSemaphore_r);
            } else if (pin == BTN_PIN_Y) {
                xSemaphoreGive(xSemaphore_y);
            }
        }
    }
}

void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int blinking = 0;
    while (true) {
        if (xSemaphoreTake(xSemaphore_r, blinking ? pdMS_TO_TICKS(100) : portMAX_DELAY) == pdTRUE) {
            blinking = !blinking;
            if (!blinking) {
                gpio_put(LED_PIN_R, 0);
            }
        }

        if (blinking) {
            gpio_put(LED_PIN_R, !gpio_get(LED_PIN_R));
        }
    }
}

void led_y_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    int blinking = 0;
    while (true) {
        if (xSemaphoreTake(xSemaphore_y, blinking ? pdMS_TO_TICKS(100) : portMAX_DELAY) == pdTRUE) {
            blinking = !blinking;
            if (!blinking) {
                gpio_put(LED_PIN_Y, 0);
            }
        }

        if (blinking) {
            gpio_put(LED_PIN_Y, !gpio_get(LED_PIN_Y));
        }
    }
}

int main() {
    stdio_init_all();

    xQueueBtn = xQueueCreate(32, sizeof(int));
    xSemaphore_r = xSemaphoreCreateBinary();
    xSemaphore_y = xSemaphoreCreateBinary();

    xTaskCreate(btn_task, "BTN_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_r_task, "LED_R_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "LED_Y_Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {}

    return 0;
}