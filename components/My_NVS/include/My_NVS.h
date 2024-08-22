#ifndef _MY_NVS_H_
#define _MT_NVS_H_

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t NVS_init(void);
esp_err_t NVS_Write(uint8_t unit, float set);
esp_err_t NVS_Read(uint8_t *unit, float *set);

#ifdef __cplusplus
}
#endif

#endif