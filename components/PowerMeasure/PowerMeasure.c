#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "PowerMeasure.h"

static const char *TAG = "PowerMeasure";

int Battery = 0;
#define PowerAvgtimes 10
int BatteryTemp[PowerAvgtimes];
static adc_oneshot_unit_handle_t handle;
static adc_channel_t ADC_CHANNEL;
int16_t ADC_OFFSET;
float CONVERTSION_FACTOR;
esp_err_t PowerMeasureinit(adc_channel_t adc_CHANNEL, int16_t adc_Lowpower, int16_t adc_maxpower)
{
    esp_err_t ret;

    adc_oneshot_unit_init_cfg_t init_cofig = {ADC_UNIT_1, ADC_ULP_MODE_DISABLE};
    adc_oneshot_chan_cfg_t adc_config = {ADC_ATTEN_DB_0, ADC_BITWIDTH_12};
    ADC_CHANNEL = adc_CHANNEL;
    ADC_OFFSET = adc_Lowpower;
    CONVERTSION_FACTOR = 100.0f / (adc_maxpower - adc_Lowpower);

    ret = adc_oneshot_new_unit(&adc_config, &handle);
    if (ret != ESP_OK)
        return ret;
    ret = adc_oneshot_config_channel(handle, ADC_CHANNEL, &adc_config);
    if (ret != ESP_OK)
        return ret;
    ESP_LOGI(TAG, "Power Measure init.");
    xTaskCreate(Measure, "Measure", 5 * 1024, NULL, 5, NULL);
    return ESP_OK;
}

void Measure(void *Measure)
{
    int adcvlaue;
    int c = 0;
    adc_oneshot_read(handle, ADC_CHANNEL, &adcvlaue);
    int V = (adcvlaue - ADC_OFFSET) * CONVERTSION_FACTOR;
    V = (100 < V) ? 100 : V;
    V = (0 > V) ? 0 : V;
    for (int i = 0; i < PowerAvgtimes; i++)
    {
        BatteryTemp[i] = V;
    }
    while (1)
    {
        c++;
        if (c > PowerAvgtimes)
            c = 0;
        adc_oneshot_read(handle, ADC_CHANNEL, &adcvlaue);
        V = (adcvlaue - ADC_OFFSET) * CONVERTSION_FACTOR;
        V = (100 < V) ? 100 : V;
        V = (0 > V) ? 0 : V;
        BatteryTemp[c] = V;
        long avg = 0;
        for (int i = 0; i < PowerAvgtimes; i++)
        {
            avg += BatteryTemp[i];
        }
        Battery = avg / PowerAvgtimes;
        if (Battery <= 0)
        {
            ESP_LOGI(TAG, "Low battery level");
        }
        // ESP_LOGI(TAG, "Battery:%d", Battery);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int get_Power()
{
    return Battery;
}