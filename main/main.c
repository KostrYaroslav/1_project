#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "wifi_app.h"
#include "my_blink.h"

int global_blink_delay = 1000; 
char global_hostname[32] = "esp32"; // Буфер для имени

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
        // Читаем задержку
        int32_t saved_delay = 1000;
        if (nvs_get_i32(my_handle, "delay", &saved_delay) == ESP_OK) {
            global_blink_delay = (int)saved_delay;
        }
        // ЧИТАЕМ ИМЯ ХОСТА
        size_t required_size = sizeof(global_hostname);
        if (nvs_get_str(my_handle, "hostname", global_hostname, &required_size) != ESP_OK) {
            strcpy(global_hostname, "esp32"); // Дефолт, если пусто
        }
        nvs_close(my_handle);
    }

    wifi_app_start();
    run_blink_logic(); 
}
