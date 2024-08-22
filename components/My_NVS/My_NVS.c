#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "nvs.h"

#include "My_NVS.h"
static const char *TAG = "NVS";
esp_err_t NVS_init(void)
{
    // Initialize NVS
    ESP_LOGI(TAG, "Initialize NVS!");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}
esp_err_t NVS_Write(uint8_t unit, float set)
{
    esp_err_t ret;
    nvs_handle_t my_handle;
    uint32_t *se;
    ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK)
        return ret;
    ret = nvs_set_u8(my_handle, "unit", unit);
    if (ret != ESP_OK)
        return ret;
    se = (uint32_t *)&set;
    ret = nvs_set_u32(my_handle, "set", *se);
    if (ret != ESP_OK)
        return ret;
    nvs_close(my_handle);
    return ret;
}
esp_err_t NVS_Read(uint8_t *unit, float *set)
{
    esp_err_t ret;
    nvs_handle_t my_handle;
    ret = nvs_open("storage", NVS_READONLY, &my_handle);
    if (ret != ESP_OK)
        return ret;
    ret = nvs_get_u8(my_handle, "unit", unit);
    if (ret != ESP_OK)
        return ret;
    ret = nvs_get_u32(my_handle, "set", (uint32_t *)set);
    if (ret != ESP_OK)
        return ret;
    nvs_close(my_handle);
    return ret;
}