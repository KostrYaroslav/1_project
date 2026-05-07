idf_component_register(SRCS "wifi_app.c" "web_server.c" "ota_handler.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi esp_event esp_http_server nvs_flash app_update bootloader_support espressif__mdns)

# Путь к HTML (если папка html лежит в main)
target_add_binary_data(${COMPONENT_TARGET} "../../main/html/index.html" TEXT)
