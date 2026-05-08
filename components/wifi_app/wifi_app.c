#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "mdns.h"
#include "wifi_app.h"
#include "wifi_app_internal.h"

static const char *TAG = "WIFI_APP";
extern char global_hostname[];
static int s_retry_num = 0;
static bool s_tried_fallback = false;
static bool s_searching_open = false; // Флаг поиска открытых сетей

void get_config_param(const char* key, char* out_val, const char* default_val) {
    nvs_handle_t h;
    if (nvs_open("storage", NVS_READONLY, &h) == ESP_OK) {
        size_t size = 32;
        if (nvs_get_str(h, key, out_val, &size) != ESP_OK) strcpy(out_val, default_val);
        nvs_close(h);
    } else strcpy(out_val, default_val);
}

// Функция для поиска и подключения к открытой сети
static void connect_to_open_network(void) {
    ESP_LOGW(TAG, "Поиск открытых сетей...");
    wifi_scan_config_t scan_config = { .ssid = 0, .bssid = 0, .channel = 0, .show_hidden = false };
    esp_wifi_scan_start(&scan_config, true);

    uint16_t number = 10;
    wifi_ap_record_t ap_info[10];
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_records(&number, ap_info);
    esp_wifi_scan_get_ap_num(&ap_count);

    for (int i = 0; i < ap_count; i++) {
        if (ap_info[i].authmode == WIFI_AUTH_OPEN) {
            ESP_LOGI(TAG, "Найдена открытая сеть: %s. Пробуем подключиться...", ap_info[i].ssid);
            wifi_config_t open_cfg = {0};
            strncpy((char*)open_cfg.sta.ssid, (char*)ap_info[i].ssid, 32);
            esp_wifi_set_config(WIFI_IF_STA, &open_cfg);
            esp_wifi_connect();
            return;
        }
    }
    ESP_LOGE(TAG, "Открытых сетей не найдено. Начинаем цикл заново.");
    s_retry_num = 0;
    s_tried_fallback = false;
    s_searching_open = false;
    esp_restart(); // Рестарт для чистого цикла опроса
}

static void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Попытка %d к целевой сети...", s_retry_num);
        } else if (!s_tried_fallback) {
            // ШАГ 2: Переход к "соседней" сети
            s_tried_fallback = true;
            s_retry_num = 0;
            char active[10], ssid[32], pass[64];
            get_config_param("active_net", active, "main");
            
            if (strcmp(active, "main") == 0) {
                ESP_LOGW(TAG, "Основная не ответила. Пробуем Резервную...");
                get_config_param("r_ssid", ssid, CONFIG_WIFI_RESERVE_SSID);
                get_config_param("r_pass", pass, CONFIG_WIFI_RESERVE_PASS);
            } else {
                ESP_LOGW(TAG, "Резервная не ответила. Пробуем Основную...");
                get_config_param("m_ssid", ssid, CONFIG_WIFI_SSID);
                get_config_param("m_pass", pass, CONFIG_WIFI_PASS);
            }
            wifi_config_t fb_cfg = {0};
            strncpy((char*)fb_cfg.sta.ssid, ssid, 32);
            strncpy((char*)fb_cfg.sta.password, pass, 64);
            esp_wifi_set_config(WIFI_IF_STA, &fb_cfg);
            esp_wifi_connect();
        } else if (!s_searching_open) {
            // ШАГ 3: Поиск открытой сети
            s_searching_open = true;
            connect_to_open_network();
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_retry_num = 0;
        s_tried_fallback = false;
        s_searching_open = false;
        mdns_init();
        mdns_hostname_set(global_hostname);
        start_webserver();
    }
}

void wifi_app_start(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    char ssid[32], pass[64], active[10];
    get_config_param("active_net", active, "main");
    
    if (strcmp(active, "res") == 0) {
        get_config_param("r_ssid", ssid, CONFIG_WIFI_RESERVE_SSID);
        get_config_param("r_pass", pass, CONFIG_WIFI_RESERVE_PASS);
    } else {
        get_config_param("m_ssid", ssid, CONFIG_WIFI_SSID);
        get_config_param("m_pass", pass, CONFIG_WIFI_PASS);
    }

    wifi_config_t w_cfg = {0};
    strncpy((char*)w_cfg.sta.ssid, ssid, 32);
    strncpy((char*)w_cfg.sta.password, pass, 64);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &w_cfg);
    esp_wifi_start();
}
