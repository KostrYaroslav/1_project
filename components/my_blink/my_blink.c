#include <stdio.h>
#include "my_blink.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_PIN 2
extern int global_blink_delay; 

void run_blink_logic(void) {
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_INPUT_OUTPUT);
    
    while(1) {
        if (global_blink_delay < 10) {
            // Режим фонарика: просто включаем LED
            gpio_set_level(LED_PIN, 1);
            // Важно: небольшая задержка, чтобы FreeRTOS могла переключиться на Wi-Fi задачи
            vTaskDelay(pdMS_TO_TICKS(100)); 
        } 
        else {
            // Режим мигалки
            gpio_set_level(LED_PIN, !gpio_get_level(LED_PIN));
            vTaskDelay(pdMS_TO_TICKS(global_blink_delay));
        }
    }
}
