#ifndef _POWERKEY_H_
#define _POWERKEY_H_
#include "driver/gpio.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct keypower_config_t {
    gpio_num_t button_gpio_pin;     // 按键GPIO引脚
    gpio_num_t power_gpio_pin;      // 电源GPIO引脚
    uint16_t power_on_press_duration;   // 开机长按时间
    uint16_t power_off_press_duration;  // 关机长按时间
} keypower_config_t;
uint8_t get_powerstatic();
esp_err_t powerKey_init(const keypower_config_t *config);

#ifdef __cplusplus
}
#endif
#endif 