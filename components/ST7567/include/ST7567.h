#ifndef _ST7567_H_
#define _ST7567_H_

#include "driver/spi_common.h"

#ifdef __cplusplus
extern "C" {
#endif
//#define ST_7567_FRAME_BUF_SIZE 504
//#define ST_7567_TRANS_QUEUE_SIZE 504

typedef struct ST_7567_config_t {
    uint8_t mosi_io_num;
    uint8_t sclk_io_num;
    uint8_t cs_io_num;
    uint8_t reset_io_num;
    uint8_t dc_io_num;
    uint8_t backlight_io_num;
} ST_7567_config_t;
esp_err_t ST_7567_init(const ST_7567_config_t *config);
void ST_7567_printf(int x,int y,const char *format, ...);
void ST_7567_printf16(int x,int y,const char *format, ...);
void ST_7567_display(int x, int y, uint8_t *data, int data_len);
void ST_7567_display16(int x, int y, uint8_t *data, int data_len);
void ST_7567_Poweroff();
#ifdef __cplusplus
}
#endif

#endif 
