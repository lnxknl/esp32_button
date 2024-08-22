#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ST7567.h"
#include "font5x7.h"
#include "font16x16.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "ST_7567";
#define FRAME_BUF_LENGTH 128 * 64 / 8
static uint8_t frame_buf[FRAME_BUF_LENGTH];
static spi_device_handle_t spi;
static uint8_t DC_pin;
static uint8_t Light_pin;
static uint8_t data_swap_u8(uint8_t data)
{
    data = ((data >> 1u) & 0x55u) | ((data & 0x55u) << 1u);
    data = ((data >> 2u) & 0x33u) | ((data & 0x33u) << 2u);
    data = ((data >> 4u) & 0x0fu) | ((data & 0x0fu) << 4u);
    return data;
}
static void ST_7567_queue_trans(const uint8_t *cmds_or_data, int len, bool dc)
{
    size_t length = 8 * sizeof(uint8_t) * len;
    spi_transaction_t *t = malloc(sizeof(spi_transaction_t));
    memset(t, 0, sizeof(spi_transaction_t));
    t->length = length;
    t->tx_buffer = cmds_or_data;
    t->flags = 0;
    gpio_set_level(DC_pin, (dc ? 1 : 0));
    // ESP_ERROR_CHECK( spi_device_queue_trans(spi, t, portMAX_DELAY) );
    ESP_ERROR_CHECK(spi_device_transmit(spi, t));
    free(t);
}

static void ST_7567_cmds(const uint8_t *cmds, int len)
{
    ST_7567_queue_trans(cmds, len, false);
}

static void ST_7567_data(const uint8_t *data, int len)
{
    ST_7567_queue_trans(data, len, true);
}
// void ST_7567_set_pos(uint8_t row, uint8_t col)
// {
//     uint8_t pos[3];

//     pos[0] = data_swap_u8(0xb0 | (0x07 & row)),
//     pos[1] = data_swap_u8(0x0f & col);
//     pos[2] = data_swap_u8(0x10 | (col >> 4));
//     ST_7567_cmds(&pos[0], 3);
// }

void ST_7567_set_pos(uint8_t x, uint8_t y)
{
    uint8_t pos[3];
    pos[0] = (0xb0 | y);//1 0 1 1 Y3 Y2 Y1 Y0 Set page address 
    pos[1] = (0x0f & x);//0 0 0 0 X3 X2 X1 X0 Set column address (LSB)
    pos[2] = (0x10 | (x >> 4));//0 0 0 1 X7 X6 X5 X4 Set column address (MSB)
    ST_7567_cmds(&pos[0], 3);
}
void ST_7567_clear_display()
{
    for (int i = 0; i < 8; i++)
    {
        ST_7567_set_pos(0, i);
        memset(frame_buf, 0, 128);
        ST_7567_data(frame_buf, 128);
    }
}

void ST_7567_puts(int x, int y, const char *text)
{
    uint8_t text_len = strlen(text);
    uint8_t char_width = 6;
    uint16_t data_len = char_width * text_len;
    uint8_t *data = &frame_buf[0];
    for (uint8_t i = 0; i < text_len; i++)
    {
        for (uint8_t j = 0; j < char_width - 1; j++)
        {
            data[i * char_width + j] = font5x7[text[i] - FONT5X7_CHAR_CODE_OFFSET][j];
        }
        data[i * char_width + 5] = 0;
    }
    ST_7567_set_pos(x, y);
    ST_7567_data(&data[0], data_len);
}

void ST_7567_printf(int x, int y, const char *format, ...)
{
    char s[64];
    va_list ap;
    va_start(ap, format);
    vsprintf(s, format, ap);
    va_end(ap);
    ST_7567_puts(x, y, s);
}

void ST_7567_puts16(int x, int y, const char *text)
{
    uint8_t text_len = strlen(text);
    uint8_t char_width = 16;
    uint16_t data_len = char_width * text_len;
    for (uint8_t i = 0; i < text_len; i++)
    {
        for (uint8_t j = 0; j < (char_width / 2); j++)
        {
            frame_buf[i * (char_width / 2) + j] = font16x16[text[i] - FONT16x16_CHAR_CODE_OFFSET][j];
        }
    }
    for (uint8_t i = 0; i < text_len; i++)
    {
        for (uint8_t j = 0; j < (char_width / 2); j++)
        {
            frame_buf[data_len / 2 + i * (char_width / 2) + j] = font16x16[text[i] - FONT16x16_CHAR_CODE_OFFSET][(char_width / 2) + j];
        }
    }
    ST_7567_set_pos(x, y);
    ST_7567_data(&frame_buf[0], data_len / 2);
    ST_7567_set_pos(x, y + 1);
    ST_7567_data(&frame_buf[data_len / 2], data_len / 2);
}
void ST_7567_printf16(int x, int y, const char *format, ...)
{
    char s[64];
    va_list ap;
    va_start(ap, format);
    vsprintf(s, format, ap);
    va_end(ap);
    ST_7567_puts16(x, y, s);
}
void ST_7567_display(int x, int y, uint8_t *data, int data_len)
{
    ST_7567_set_pos(x, y);
    ST_7567_data(data, data_len);
}
void ST_7567_display16(int x, int y, uint8_t *data, int data_len)
{
    ST_7567_set_pos(x, y);
    ST_7567_data(data, data_len / 2);
    ST_7567_set_pos(x, y + 1);
    ST_7567_data(data + data_len / 2, data_len / 2);
}
void ST_7567_Poweroff()
{
    ST_7567_clear_display();
    gpio_set_level(Light_pin, 0);
}
esp_err_t ST_7567_init(const ST_7567_config_t *config)
{
    if (!config)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;

    // 配置SPI总线
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = config->mosi_io_num,
        .sclk_io_num = config->sclk_io_num,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1};
    ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, SPI_DMA_CH1);
    if (ret != ESP_OK)
        return ret;

    // 配置SPI设备
    spi_device_interface_config_t dev_cfg = {
        .mode = 0,
        .clock_speed_hz = 4 * 1000 * 1000,
        .spics_io_num = config->cs_io_num,
        .queue_size = 1,
        //.flags = SPI_DEVICE_BIT_LSBFIRST
    };

    ret = spi_bus_add_device(HSPI_HOST, &dev_cfg, &spi);
    if (ret != ESP_OK)
        return ret;

    ESP_LOGI(TAG, "SPI pins: Din=IO%d, Clk=IO%d, CE=IO%d",
             config->mosi_io_num, config->sclk_io_num, config->cs_io_num);

    // 配置显示屏控制引脚
    gpio_reset_pin(config->dc_io_num);
    gpio_set_direction(config->dc_io_num, GPIO_MODE_OUTPUT);
    DC_pin = config->dc_io_num;
    gpio_reset_pin(config->reset_io_num);
    gpio_set_direction(config->reset_io_num, GPIO_MODE_OUTPUT);

    gpio_reset_pin(config->backlight_io_num);
    gpio_set_direction(config->backlight_io_num, GPIO_MODE_OUTPUT);
    Light_pin = config->backlight_io_num;
    ESP_LOGI(TAG, "Control pins: RST=IO%d, DC=IO%d, BL=IO%d",
             config->reset_io_num, config->dc_io_num, config->backlight_io_num);

    gpio_set_level(config->reset_io_num, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    // 复位显示屏
    gpio_set_level(config->reset_io_num, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(config->reset_io_num, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    // 配置显示屏
    spi_device_acquire_bus(spi, portMAX_DELAY);
    // spi_device_release_bus(spi);
    uint8_t cmds[8];
    cmds[0] = 0b00101111;//0 0 1 0 1 VB VR VF Control buil
    cmds[1] = 0b10000001;//1 0 0 0 0 0 0 1 Double command!! Set electronic volume (EV) level
    cmds[2] = 0b00100000;//0 0 EV5 EV4 EV3 EV2 EV1 EV0
    cmds[3] = 0b10100010;//1 0 1 0 0 0 1 BS Select bias setting 0=1/9; 1=1/7 (at 1/65 duty) 
    cmds[4] = 0b01000000;//0 1 S5 S4 S3 S2 S1 S0 Set display start line
    cmds[5] = 0b10100000;//1 0 1 0 0 0 0 MX Set scan direction of SEG MX=1, reverse direction MX=0, normal direction 
    cmds[6] = 0b11001000;//1 1 0 0 MY - - - Set output direction of COM MY=1, reverse direction MY=0, normal direction
    cmds[7] = 0b10101111;//1 0 1 0 1 1 1 D D=1, display ON D=0, display OFF 

    ST_7567_queue_trans(cmds, 8, false);

    // malloc(ST_7567_TRANS_QUEUE_SIZE * sizeof(void*));
    // frame_buf = heap_caps_malloc(ST_7567_FRAME_BUF_SIZE, MALLOC_CAP_DMA);
    ST_7567_clear_display();
    ESP_LOGI(TAG, "ST_7567_init");
    gpio_set_level(config->backlight_io_num, 1);
    return ESP_OK;
}