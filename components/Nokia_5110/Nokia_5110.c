#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Nokia_5110.h"
#include "font5x7.h"
#include "font16x16.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "Nokia_5110";
#define FRAME_BUF_LENGTH 504
static uint8_t frame_buf[FRAME_BUF_LENGTH];
static spi_device_handle_t spi;
static uint8_t DC_pin;
static uint8_t Light_pin;
static void Nokia_5110_queue_trans(const uint8_t *cmds_or_data, int len, bool dc)
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

static void Nokia_5110_cmds(const uint8_t *cmds, int len)
{
    Nokia_5110_queue_trans(cmds, len, false);
}

static void Nokia_5110_data(const uint8_t *data, int len)
{
    Nokia_5110_queue_trans(data, len, true);
}
void Nokia_5110_set_pos(uint8_t row, uint8_t col)
{
    uint8_t pos[2];
    pos[0] = 0b10000000 | row,
    pos[1] = 0b01000000 | col;
    Nokia_5110_cmds(&pos[0], 2);
}
void Nokia_5110_clear_display()
{
    Nokia_5110_set_pos(0, 0);
    memset(frame_buf, 0, FRAME_BUF_LENGTH);
    Nokia_5110_data(frame_buf, FRAME_BUF_LENGTH);
}

void Nokia_5110_puts(int x, int y, const char *text)
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
    Nokia_5110_set_pos(x, y);
    Nokia_5110_data(&data[0], data_len);
}

void Nokia_5110_printf(int x, int y, const char *format, ...)
{
    char s[64];
    va_list ap;
    va_start(ap, format);
    vsprintf(s, format, ap);
    va_end(ap);
    Nokia_5110_puts(x, y, s);
}

void Nokia_5110_puts16(int x, int y, const char *text)
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
    Nokia_5110_set_pos(x, y);
    Nokia_5110_data(&frame_buf[0], data_len / 2);
    Nokia_5110_set_pos(x, y + 1);
    Nokia_5110_data(&frame_buf[data_len / 2], data_len / 2);
}
void Nokia_5110_printf16(int x, int y, const char *format, ...)
{
    char s[64];
    va_list ap;
    va_start(ap, format);
    vsprintf(s, format, ap);
    va_end(ap);
    Nokia_5110_puts16(x, y, s);
}
void Nokia_5110_display(int x, int y, uint8_t *data, int data_len)
{
    Nokia_5110_set_pos(x, y);
    Nokia_5110_data(data, data_len);
}
void Nokia_5110_display16(int x, int y, uint8_t *data, int data_len)
{
    Nokia_5110_set_pos(x, y);
    Nokia_5110_data(data, data_len / 2);
    Nokia_5110_set_pos(x, y + 1);
    Nokia_5110_data(data+data_len / 2, data_len / 2);
}
void Nokia_5110_Poweroff()
{
    Nokia_5110_clear_display();
    gpio_set_level(Light_pin, 0);
}
esp_err_t nokia_5110_init(const nokia_5110_config_t *config)
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
        .queue_size = 1};

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
    uint8_t cmds[5];
    cmds[0] = 0b00100001;      // extended instruction set
    cmds[1] = 0b00000100 | 7;  // templerature coefficient
    cmds[2] = 0b00010000 | 2;  // bias system
    cmds[3] = 0b10000000 | 80; // Vop (contrast)
    cmds[4] = 0b00100000;
    cmds[5] = 0b00001100;
    Nokia_5110_queue_trans(cmds, 6, false);
    // malloc(NOKIA_5110_TRANS_QUEUE_SIZE * sizeof(void*));
    // frame_buf = heap_caps_malloc(NOKIA_5110_FRAME_BUF_SIZE, MALLOC_CAP_DMA);
    Nokia_5110_clear_display();
    ESP_LOGI(TAG, "nokia_5110_init");
    gpio_set_level(config->backlight_io_num, 1);
    return ESP_OK;
}