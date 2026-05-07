#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"      // Добавили для работы с функциями NVS
#include "wifi_app.h"
#include "my_blink.h"

// Глобальная переменная
int global_blink_delay = 1000; 

void app_main(void) {
    // 1. Инициализация системной области NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 2. ЧТЕНИЕ СОХРАНЕННОЙ ЗАДЕРЖКИ ПРИ СТАРТЕ
    nvs_handle_t my_handle;
    // Открываем пространство имен "storage" в режиме только чтение
    if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK) {
        int32_t saved_delay = 1000; 
        // Пробуем прочитать значение по ключу "delay"
        if (nvs_get_i32(my_handle, "delay", &saved_delay) == ESP_OK) {
            global_blink_delay = (int)saved_delay;
            printf("Успешно загружена задержка из памяти: %d мс\n", global_blink_delay);
        }
        nvs_close(my_handle);
    }

    // 3. Запуск Wi-Fi и логики мигания
    wifi_app_start();
    run_blink_logic(); 
}
