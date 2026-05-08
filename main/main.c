#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "wifi_app.h"
#include "my_blink.h"

// Глобальные переменные
int global_blink_delay = 1000; 
char global_hostname[32] = "esp32"; // Исправлено: массив символов

void app_main(void) {
    // 1. Инициализация NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 2. Чтение настроек из памяти
    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
        // Задержка
        int32_t saved_delay = 1000;
        if (nvs_get_i32(my_handle, "delay", &saved_delay) == ESP_OK) {
            global_blink_delay = (int)saved_delay;
        }
        // Имя mDNS
        size_t size = sizeof(global_hostname);
        if (nvs_get_str(my_handle, "hostname", global_hostname, &size) != ESP_OK) {
            strcpy(global_hostname, "esp32");
        }
        nvs_close(my_handle);
    }

    // 3. Запуск систем
    wifi_app_start();
    run_blink_logic(); 
}
