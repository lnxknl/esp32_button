idf_component_register(SRCS  "button_gpio.c" "iot_button.c"
                        INCLUDE_DIRS include
                        REQUIRES driver esp_timer)
