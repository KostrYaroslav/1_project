#include <stdio.h>
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "wifi_app_internal.h"

static const char *TAG = "OTA_HANDLER";

esp_err_t update_post_handler(httpd_req_t *req) {
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    char *buf = malloc(1024);
    int received, total = 0;
    bool started = false;

    while ((received = httpd_req_recv(req, buf, 1024)) > 0) {
        if (!started) {
            if ((uint8_t)buf[0] != 0xE9) { 
                ESP_LOGE(TAG, "Magic byte error!");
                free(buf); return ESP_FAIL; 
            }
            esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
            started = true;
        }
        esp_ota_write(update_handle, buf, received);
        total += received;
    }
    free(buf);
    esp_ota_end(update_handle);
    esp_ota_set_boot_partition(update_partition);
    ESP_LOGI(TAG, "OTA Done: %d bytes", total);
    httpd_resp_sendstr(req, "Успех! Перезагрузка...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
    return ESP_OK;
}
