#include <stdio.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "wifi_app_internal.h"

static const char *TAG = "WEB_SERVER";

extern int global_blink_delay;
extern char global_hostname[32]; 
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

static esp_err_t index_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
}

static esp_err_t status_get_handler(httpd_req_t *req) {
    char json[512]; 
    snprintf(json, sizeof(json), 
             "{\"mem\":%lu,\"up\":%lu,\"del\":%d,\"host\":\"%s\",\"ssid\":\"%s\",\"pass\":\"%s\",\"r_ssid\":\"%s\",\"r_pass\":\"%s\"}", 
             esp_get_free_heap_size()/1024, esp_log_timestamp()/1000, global_blink_delay,
             global_hostname, CONFIG_WIFI_SSID, CONFIG_WIFI_PASS, 
             CONFIG_WIFI_RESERVE_SSID, CONFIG_WIFI_RESERVE_PASS);
             
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t set_handler(httpd_req_t *req) {
    char buf[128];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char param[32];
        if (httpd_query_key_value(buf, "val", param, sizeof(param)) == ESP_OK) {
            global_blink_delay = atoi(param);
            nvs_handle_t h;
            if (nvs_open("storage", NVS_READWRITE, &h) == ESP_OK) {
                nvs_set_i32(h, "delay", (int32_t)global_blink_delay);
                nvs_commit(h);
                nvs_close(h);
            }
        }
    }
    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t host_set_handler(httpd_req_t *req) {
    char buf[128];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char param[32];
        if (httpd_query_key_value(buf, "val", param, sizeof(param)) == ESP_OK) {
            nvs_handle_t h;
            if (nvs_open("storage", NVS_READWRITE, &h) == ESP_OK) {
                nvs_set_str(h, "hostname", param);
                nvs_commit(h);
                nvs_close(h);
            }
        }
    }
    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t restart_get_handler(httpd_req_t *req) {
    httpd_resp_sendstr(req, "OK");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

void start_webserver(void) {
    static httpd_handle_t server = NULL;
    if (server) return;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_index   = { .uri = "/",        .method = HTTP_GET,  .handler = index_get_handler };
        httpd_uri_t uri_status  = { .uri = "/status",  .method = HTTP_GET,  .handler = status_get_handler };
        httpd_uri_t uri_set     = { .uri = "/set",      .method = HTTP_GET,  .handler = set_handler };
        httpd_uri_t uri_host    = { .uri = "/set_host", .method = HTTP_GET,  .handler = host_set_handler };
        httpd_uri_t uri_restart = { .uri = "/restart",  .method = HTTP_GET,  .handler = restart_get_handler };
        httpd_uri_t uri_upd     = { .uri = "/update",   .method = HTTP_POST, .handler = update_post_handler };
        httpd_register_uri_handler(server, &uri_index);
        httpd_register_uri_handler(server, &uri_status);
        httpd_register_uri_handler(server, &uri_set);
        httpd_register_uri_handler(server, &uri_host);
        httpd_register_uri_handler(server, &uri_restart);
        httpd_register_uri_handler(server, &uri_upd);
    }
}
