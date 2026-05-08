#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_http_server.h" 
#include "esp_err.h"         
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

// Ссылка на функцию из wifi_app.c
extern void get_config_param(const char* key, char* out_val, const char* default_val);

static esp_err_t index_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
}

static esp_err_t status_get_handler(httpd_req_t *req) {
    char m_ssid[32], m_pass[64], r_ssid[32], r_pass[64], active[10];
    get_config_param("m_ssid", m_ssid, CONFIG_WIFI_SSID);
    get_config_param("m_pass", m_pass, CONFIG_WIFI_PASS);
    get_config_param("r_ssid", r_ssid, CONFIG_WIFI_RESERVE_SSID);
    get_config_param("r_pass", r_pass, CONFIG_WIFI_RESERVE_PASS);
    get_config_param("active_net", active, "main");

    char *json = malloc(512);
    snprintf(json, 512, 
             "{\"mem\":%lu,\"up\":%lu,\"del\":%d,\"host\":\"%s\",\"s1\":\"%s\",\"p1\":\"%s\",\"s2\":\"%s\",\"p2\":\"%s\",\"act\":\"%s\"}", 
             esp_get_free_heap_size()/1024, esp_log_timestamp()/1000, global_blink_delay,
             global_hostname, m_ssid, m_pass, r_ssid, r_pass, active);
             
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    free(json);
    return ESP_OK;
}

static esp_err_t wifi_set_handler(httpd_req_t *req) {
    char buf[256];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char id[10] = {0}, ssid[32] = {0}, pass[64] = {0}, act[10] = {0};
        nvs_handle_t h;
        nvs_open("storage", NVS_READWRITE, &h);
        
        if (httpd_query_key_value(buf, "id", id, sizeof(id)) == ESP_OK) {
            if (httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid)) == ESP_OK) 
                nvs_set_str(h, strcmp(id, "main") == 0 ? "m_ssid" : "r_ssid", ssid);
            if (httpd_query_key_value(buf, "pass", pass, sizeof(pass)) == ESP_OK) 
                nvs_set_str(h, strcmp(id, "main") == 0 ? "m_pass" : "r_pass", pass);
            if (httpd_query_key_value(buf, "act", act, sizeof(act)) == ESP_OK)
                nvs_set_str(h, "active_net", strcmp(act, "true") == 0 ? id : "main");
        }
        nvs_commit(h); nvs_close(h);
    }
    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t set_handler(httpd_req_t *req) {
    char buf[64];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char val[10];
        if (httpd_query_key_value(buf, "val", val, sizeof(val)) == ESP_OK) {
            global_blink_delay = atoi(val);
            nvs_handle_t h;
            nvs_open("storage", NVS_READWRITE, &h);
            nvs_set_i32(h, "delay", (int32_t)global_blink_delay);
            nvs_commit(h); nvs_close(h);
        }
    }
    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t host_set_handler(httpd_req_t *req) {
    char buf[64];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char val[32];
        if (httpd_query_key_value(buf, "val", val, sizeof(val)) == ESP_OK) {
            nvs_handle_t h;
            nvs_open("storage", NVS_READWRITE, &h);
            nvs_set_str(h, "hostname", val);
            nvs_commit(h); nvs_close(h);
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
    config.max_uri_handlers = 12;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uris[] = {
            { .uri = "/",        .method = HTTP_GET,  .handler = index_get_handler },
            { .uri = "/status",  .method = HTTP_GET,  .handler = status_get_handler },
            { .uri = "/set",      .method = HTTP_GET,  .handler = set_handler },
            { .uri = "/set_host", .method = HTTP_GET,  .handler = host_set_handler },
            { .uri = "/set_wifi", .method = HTTP_GET,  .handler = wifi_set_handler },
            { .uri = "/restart",  .method = HTTP_GET,  .handler = restart_get_handler },
            { .uri = "/update",   .method = HTTP_POST, .handler = update_post_handler }
        };
        for (int i = 0; i < sizeof(uris)/sizeof(uris[0]); i++) httpd_register_uri_handler(server, &uris[i]);
    }
}
