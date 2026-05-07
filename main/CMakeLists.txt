
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash wifi_app my_blink esp_https_ota bootloader_support)
