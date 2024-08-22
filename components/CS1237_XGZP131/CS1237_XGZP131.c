#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "CS1237_XGZP131.h"
static const char *TAG = "CS1237_XGZP131";
#define TemperatureAverageTimes 10
#define PressureAverageTimes 10
#define MaxWaitTimes 1000
#define A 23.6f
#define Ya 588000
static gpio_num_t _sck;
static gpio_num_t _dout;
static uint8_t ADC_HZ;
static uint8_t GAIN;
static int32_t ADCData;
static int32_t TeData;
static float Te;
static float Pt;
static int32_t Tdate[TemperatureAverageTimes];
static int32_t Adata[PressureAverageTimes];
void send_clk_pulses(uint8_t count_)
{
    // send the clock cycles
    for (uint8_t i = 0; i < count_; i++)
    {
        gpio_set_level(_sck, 1);
        usleep(1);
        gpio_set_level(_sck, 0);
        usleep(1);
    }
    return;
}
int32_t adc_read()
{
    int32_t adc_reading_ = 0;
    int cout = 0;
    gpio_set_direction(_dout, GPIO_MODE_INPUT);
    while (gpio_get_level(_dout) && cout < MaxWaitTimes)
    {
        cout++;
        vTaskDelay(1);
    }
    usleep(10);
    for (uint8_t i = 0; i < 24; i++)
    {

        gpio_set_level(_sck, 1);
        usleep(2);
        gpio_set_level(_sck, 0);
        adc_reading_ |= gpio_get_level(_dout) << (23 - i);
        usleep(2);
    }
    if (adc_reading_ > 0x7FFFFF)
        adc_reading_ -= 0xFFFFFF;
    gpio_set_direction(_dout, GPIO_MODE_OUTPUT);
    gpio_set_level(_dout, 1);
    return adc_reading_;
}

/// @brief
/// @param speed [5:4] 00=10Hz,01=40HZ,10=640HZ,11=1280HZ
/// @param gain [3:2] 00=1,01=2,10=64,11=128
/// @param channel [1:0] 00=ADC ,10=Temp
void CS1237_configure(uint8_t speed, uint8_t gain, uint8_t channel)
{

    uint8_t register_value;
    adc_read();

    send_clk_pulses(3);

    // the ADC DOUT-Pin switches to input
    send_clk_pulses(2);
    gpio_set_direction(_dout, GPIO_MODE_OUTPUT);
    for (uint8_t i = 0; i < 7; i++)
    {

        gpio_set_level(_sck, 1);
        usleep(2);
        gpio_set_level(_dout, ((0x65 >> (6 - i)) & 0b00000001));
        usleep(2);
        gpio_set_level(_sck, 0);
        usleep(2);
    }

    // send 1 clock pulse, to set the Pins of the ADCs to input
    send_clk_pulses(1);

    // send or adc_read the configuration to/of the ADCs
    gpio_set_direction(_dout, GPIO_MODE_OUTPUT);
    register_value = gain << 2 | speed << 4 | channel;
    for (uint8_t i = 0; i < 8; i++)
    {
        gpio_set_level(_sck, 1);
        usleep(2);
        gpio_set_level(_dout, ((register_value >> (7 - i)) & 0b00000001));
        usleep(2);
        gpio_set_level(_sck, 0);
        usleep(2);
    }

    // send 1 clock pulse, to set the Pins of the ADCs to output and pull high
    send_clk_pulses(1);

    // put the ADC-pins of the MCU in INPUT_Pullup mode again and return the register value
    gpio_set_direction(_dout, GPIO_MODE_INPUT);
    vTaskDelay(pdMS_TO_TICKS(100));
}

float convertTemp(uint32_t data)
{
    return data * (273.15f + A) / Ya - 273.15f;
}
float convertPressure(uint32_t data)
{
    return (data - 8000) / 200 + 100.0f;
}
uint32_t get_ADCData()
{
    return ADCData;
}
float get_Te()
{
    return Te;
}
float get_Pt()
{
    return Pt;
}
void CS1237_adc_read_task(void *t)
{
    while (1)
    {
        CS1237_configure(ADC_HZ, 0x00, 0x02); // 先设定为温度
        vTaskDelay(pdMS_TO_TICKS(200));
        for (int i = 0; i < TemperatureAverageTimes; i++)
        {
            Tdate[i] = adc_read();
            int32_t t = 0;
            for (int n = 0; n < TemperatureAverageTimes; n++)
            {
                t += Tdate[n];
            }
            TeData = t / TemperatureAverageTimes;

            vTaskDelay(5);
        }
        CS1237_configure(ADC_HZ, GAIN, 0x00);
        vTaskDelay(pdMS_TO_TICKS(200));
        for (int i = 0; i < PressureAverageTimes; i++)
        {
            Adata[i] = adc_read();
            int32_t t = 0;
            for (int n = 0; n < PressureAverageTimes; n++)
            {
                t += Adata[n];
            }
            ADCData = t / PressureAverageTimes;

            vTaskDelay(5);
        }
        Te = convertTemp(TeData);
        Pt = convertPressure(ADCData);
        ESP_LOGI(TAG, "CS1237 TD=%ld", TeData);
        ESP_LOGI(TAG, "CS1237 Te=%f", Te);
        ESP_LOGI(TAG, "CS1237 AD=%ld", ADCData);
        ESP_LOGI(TAG, "CS1237 Pt=%f", Pt);
        // ESP_LOGI(TAG, "CS1237 =%ld", ADCData);
        // 1003bkpa 8399  1005bpa  8479
        // 587900
    }
}

esp_err_t CS1237_init(gpio_num_t SCK, gpio_num_t Dout, uint8_t adc_HZ, uint8_t gain)
{
    gpio_reset_pin(SCK);
    gpio_set_direction(SCK, GPIO_MODE_OUTPUT);
    _sck = SCK;

    gpio_reset_pin(Dout);
    gpio_set_direction(Dout, GPIO_MODE_INPUT);
    _dout = Dout;
    ADC_HZ = adc_HZ;
    GAIN = gain;
    // CS1237_configure(ADC_HZ, 0x00, 0x02); // 先设定为温度
    ESP_LOGI(TAG, "CS1237 init.");
    xTaskCreate(CS1237_adc_read_task, "CS1237_adc_read_task", 1024 * 10, NULL, 3, NULL);
    return ESP_OK;
}
