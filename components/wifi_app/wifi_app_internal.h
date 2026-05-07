#pragma once
#include "esp_http_server.h"

// Функции, которые мы вынесли в отдельные файлы
void start_webserver(void);
esp_err_t update_post_handler(httpd_req_t *req);
