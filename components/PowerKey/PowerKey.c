#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "PowerKey.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "PowerKey";
static uint8_t powerstatic =0;
static void long_press_button_task(void *arg);
static void WaitPowerON(const keypower_config_t *config);
esp_err_t powerKey_init(const keypower_config_t *config)
{

    if (!config)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // 配置按键引脚
    gpio_reset_pin(config->button_gpio_pin);
    gpio_set_direction(config->button_gpio_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(config->button_gpio_pin, GPIO_PULLUP_ONLY);

    // 配置电源引脚
    gpio_reset_pin(config->power_gpio_pin);
    gpio_set_direction(config->power_gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(config->power_gpio_pin, 0);

    ESP_LOGI(TAG, "Power pins: power=IO%d, key=IO%d",
             config->power_gpio_pin, config->button_gpio_pin);
    ESP_LOGI(TAG, "Power time: on=%d, off=%d",
             config->power_on_press_duration, config->power_off_press_duration);

    WaitPowerON(config);
    // 创建任务
    xTaskCreate(long_press_button_task, "long_press_button_task", 1024*4, (void *)config, 2, NULL);
    return ESP_OK;
}

static void WaitPowerON(const keypower_config_t *config)
{
    ESP_LOGI(TAG, "Wait Power On!");

    TickType_t start_tick = 0;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 10;

    // 使用当前时间 初始化xLastWakeTime 变量
    xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        int button_state = gpio_get_level(config->button_gpio_pin);
        if (button_state == 0)
        {
            // 按键被按下
            if (start_tick == 0)
            {
                // 开始计时
                start_tick = xTaskGetTickCount();
            }
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
            if ((xTaskGetTickCount() - start_tick) >= pdMS_TO_TICKS(config->power_on_press_duration))
            {
                gpio_set_level(config->power_gpio_pin, 1);
                ESP_LOGI(TAG, "Power On!");
                powerstatic=1;
                break;
            }
        }
    }
}
static void long_press_button_task(void *arg)
{
    const keypower_config_t *config = (keypower_config_t *)arg;
    TickType_t start_tick = 0;
    while (!gpio_get_level(config->button_gpio_pin))
    {
        vTaskDelay(pdMS_TO_TICKS(100));
       
    }
    powerstatic=2;
    while (1)
    {
        // 读取按键状态
        int button_state = gpio_get_level(config->button_gpio_pin);

        if (button_state == 0)
        {
            // 按键被按下
            if (start_tick == 0)
            {
                // 开始计时
                start_tick = xTaskGetTickCount();
                ESP_LOGI(TAG, "Wait power Off!");
            }
            if ((xTaskGetTickCount() - start_tick) >= pdMS_TO_TICKS(config->power_off_press_duration))
            {
                gpio_set_level(config->power_gpio_pin, 0);
                start_tick = 0;
                ESP_LOGI(TAG, "Power Off!");
                while (1)
                {
                    powerstatic=0;
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }
        }
        else
        {
            start_tick = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // 10毫秒的延时
    }
}
uint8_t get_powerstatic()
{
    return powerstatic;
}