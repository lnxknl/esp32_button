#ifndef _POWERMEASURE_H_
#define _POWERMEASURE_H_
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t PowerMeasureinit(adc_channel_t adc_CHANNEL, int16_t adc_Lowpower, int16_t adc_maxpower);
void Measure(void *Measure);
int get_Power();
#ifdef __cplusplus
}
#endif
#endif 