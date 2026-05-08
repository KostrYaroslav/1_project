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
extern char global_hostname[]; 
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

extern void get_config_param(const char* key, char* out_val, const char* default_val);
extern void wifi_get_current_stats(char *ssid, int *rssi);

static esp_err_t index_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
}

static esp_err_t status_get_handler(httpd_req_t *req) {
    char m_ssid[32], m_pass[64], r_ssid[32], r_pass[64], active[10], cur_s[32];
    int rssi;
    
    get_config_param("m_ssid", m_ssid, CONFIG_WIFI_SSID);
    get_config_param("m_pass", m_pass, CONFIG_WIFI_PASS);
    get_config_param("r_ssid", r_ssid, CONFIG_WIFI_RESERVE_SSID);
    get_config_param("r_pass", r_pass, CONFIG_WIFI_RESERVE_PASS);
    get_config_param("active_net", active, "main");
    wifi_get_current_stats(cur_s, &rssi);

    char *json = malloc(1024);
    if (!json) return ESP_FAIL;

    snprintf(json, 1024, 
             "{\"mem\":%lu,\"up\":%lu,\"del\":%d,\"host\":\"%s\",\"s1\":\"%s\",\"p1\":\"%s\",\"s2\":\"%s\",\"p2\":\"%s\",\"act\":\"%s\",\"cur_s\":\"%s\",\"sig\":%d}", 
             (unsigned long)esp_get_free_heap_size()/1024, (unsigned long)esp_log_timestamp()/1000, 
             global_blink_delay, global_hostname, m_ssid, m_pass, r_ssid, r_pass, active, cur_s, rssi);
             
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    free(json);
    return ESP_OK;
}

static esp_err_t wifi_set_handler(httpd_req_t *req) {
    char buf[256];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char id[10]={0}, ssid[32]={0}, pass[64]={0}, act[10]={0};
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
    char buf[32], val[16];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
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
    char buf[32], val[32];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        if (httpd_query_key_value(buf, "val", val, sizeof(val)) == ESP_OK) {
            nvs_handle_t h; nvs_open("storage", NVS_READWRITE, &h);
            nvs_set_str(h, "hostname", val); nvs_commit(h); nvs_close(h);
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
    config.max_uri_handlers = 15;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_idx = {"/", HTTP_GET, index_get_handler, NULL};
        httpd_uri_t uri_st = {"/status", HTTP_GET, status_get_handler, NULL};
        httpd_uri_t uri_s = {"/set", HTTP_GET, set_handler, NULL};
        httpd_uri_t uri_w = {"/set_wifi", HTTP_GET, wifi_set_handler, NULL};
        httpd_uri_t uri_h = {"/set_host", HTTP_GET, host_set_handler, NULL};
        httpd_uri_t uri_r = {"/restart", HTTP_GET, restart_get_handler, NULL};
        httpd_uri_t uri_u = {"/update", HTTP_POST, update_post_handler, NULL};
        httpd_register_uri_handler(server, &uri_idx);
        httpd_register_uri_handler(server, &uri_st);
        httpd_register_uri_handler(server, &uri_s);
        httpd_register_uri_handler(server, &uri_w);
        httpd_register_uri_handler(server, &uri_h);
        httpd_register_uri_handler(server, &uri_r);
        httpd_register_uri_handler(server, &uri_u);
    }
}
