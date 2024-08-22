#ifndef _CS1237_XGZP131_H_
#define _CS1237_XGZP131_H_
#include "driver/gpio.h"
#include "sys/unistd.h"
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t CS1237_init(gpio_num_t SCK,gpio_num_t Dout,uint8_t adc_HZ,uint8_t gain);
float convertTemp(uint32_t data);
uint32_t get_ADCData();
float get_Te();
float get_Pt();
#ifdef __cplusplus
}
#endif
#endif 