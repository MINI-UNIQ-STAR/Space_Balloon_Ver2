idf_component_register(SRCS "telemetry_rx_main.c" "lora.c" "ssd1306.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver fatfs sdmmc vfs freertos esp_timer)
