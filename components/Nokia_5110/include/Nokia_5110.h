#ifndef _Nokia_5110_H_
#define _Nokia_5110_H_

#include "driver/spi_common.h"

#ifdef __cplusplus
extern "C" {
#endif
//#define NOKIA_5110_FRAME_BUF_SIZE 504
//#define NOKIA_5110_TRANS_QUEUE_SIZE 504

typedef struct nokia_5110_config_t {
    uint8_t mosi_io_num;
    uint8_t sclk_io_num;
    uint8_t cs_io_num;
    uint8_t reset_io_num;
    uint8_t dc_io_num;
    uint8_t backlight_io_num;
} nokia_5110_config_t;
esp_err_t nokia_5110_init(const nokia_5110_config_t *config);
void Nokia_5110_printf(int x,int y,const char *format, ...);
void Nokia_5110_printf16(int x,int y,const char *format, ...);
void Nokia_5110_display(int x, int y, uint8_t *data, int data_len);
void Nokia_5110_display16(int x, int y, uint8_t *data, int data_len);
void Nokia_5110_Poweroff();
#ifdef __cplusplus
}
#endif

#endif 
