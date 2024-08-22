/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "math.h"
#include "button.h"

#include "PowerKey.h"
#include "ST7567.h"
#include "PowerMeasure.h"
#include "CS1237_XGZP131.h"
#include "My_NVS.h"
#include "res_power.h"
#include "res_ico.h"
static const char *TAG = "Main_app";
const keypower_config_t keypower_config = {GPIO_NUM_35, GPIO_NUM_32, 1200, 1500};
const ST_7567_config_t ST_7567_config = {GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_16, GPIO_NUM_17};
#define KEY_ADD GPIO_NUM_26   //+
#define KEY_SUB GPIO_NUM_27   //-
#define KEY_POWER GPIO_NUM_35 // M
#define IO_Motor GPIO_NUM_25
#define LED_Button GPIO_NUM_23
#define LED_Out GPIO_NUM_33
#define DISPALYOFFSETX 16
#define DISPALYOFFSETY 1
void Display_start(); // display
void init_mbutton();
void Init_Motor();
void Motor_OFF();
void Motor_ON();

void Init_LED();
void LED_OFF();
static int BatteryPer;
static float PT_KPA;
static float PT_PSI;
static float PT_BAR;

static float SET_PT_KPA;
static float SET_PT_PSI;
static float SET_PT_BAR;
static float OFFSET_PT_KPA = 102;
static uint8_t default_unit[4] = {0, 1, 2, 1};
static float default_set[4] = {400.0f, 60.0f, 2.5f, 9.0f};
static uint8_t Unit_Status = 0;
static uint8_t Mode_Status = 0;
static uint8_t Set_Status = 0;
static uint8_t Set_StatusT = 0;
static uint8_t Set_StatusUintT = 0;
static float Set_StatusSetT = 0;
static uint8_t Motor_Status = 0;
static uint8_t LED_Status = 0;
extern "C" void app_main(void)
{
    Init_Motor();
    Init_LED();
    ESP_ERROR_CHECK(powerKey_init(&keypower_config));
    ESP_ERROR_CHECK(ST_7567_init(&ST_7567_config));
    ESP_ERROR_CHECK(PowerMeasureinit(ADC_CHANNEL_6, 2180, 2960));
    ESP_ERROR_CHECK(CS1237_init(GPIO_NUM_12, GPIO_NUM_14, 0x01, 0x01)); // 640hz gai =2
    ST_7567_printf16(0+DISPALYOFFSETX, 0+DISPALYOFFSETY, "Init.");
    vTaskDelay(pdMS_TO_TICKS(300));
    ESP_ERROR_CHECK(NVS_init());
    uint8_t uintt = 0;
    float sett = 0.0;
    if (NVS_Read(&uintt, &sett) != ESP_OK)
        ESP_LOGI(TAG, "NVS_Read faild");
    ESP_LOGI(TAG, "NVS_Read uintt=%d,sett=%f", uintt, sett);
    if (sett < 0.1f)
        ESP_ERROR_CHECK(NVS_Write(default_unit[0], default_set[0]));
    else
    {
        default_unit[0] = uintt;
        default_set[0] = sett;
    }
    Unit_Status=Set_StatusUintT = default_unit[0];
    Set_StatusSetT = default_set[0];
    ST_7567_printf16(0+DISPALYOFFSETX, 0+DISPALYOFFSETY, "Init..");
    vTaskDelay(pdMS_TO_TICKS(20));
    get_Pt();
    vTaskDelay(pdMS_TO_TICKS(20));
    get_Pt();
    vTaskDelay(pdMS_TO_TICKS(20));
    get_Pt();
    vTaskDelay(pdMS_TO_TICKS(20));
    get_Pt();
    vTaskDelay(pdMS_TO_TICKS(20));
    get_Pt();
    ST_7567_printf16(0+DISPALYOFFSETX, 0+DISPALYOFFSETY, "Init...");

    vTaskDelay(pdMS_TO_TICKS(20));
    get_Pt();
    vTaskDelay(pdMS_TO_TICKS(20));
    get_Pt();
    vTaskDelay(pdMS_TO_TICKS(20));
    get_Pt();
    // OFFSET_PT_KPA=get_Pt();
    Display_start();
    while (get_powerstatic() != 2)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    init_mbutton();
}

void display_BatteryPer()
{
    BatteryPer = get_Power();
    float T = get_Te();
    uint8_t *BatteryPer_p = &batteryico[0][0];
    static uint8_t low = 0;
    static uint8_t run = 0;
    if (BatteryPer > 80)
    {
        BatteryPer_p = &batteryico[0][0];
    }
    else if (BatteryPer > 60)
    {
        BatteryPer_p = &batteryico[1][0];
    }
    else if (BatteryPer > 40)
    {
        BatteryPer_p = &batteryico[2][0];
    }
    else if (BatteryPer > 20)
    {
        BatteryPer_p = &batteryico[3][0];
    }
    else if (BatteryPer <= 20)
    {
        if (low == 0)
            BatteryPer_p = &batteryico[4][0];
        else
            BatteryPer_p = &batteryico[5][0];
        low = !low;
    }
    ST_7567_display(0+DISPALYOFFSETX, 0+DISPALYOFFSETY, BatteryPer_p, 24);
    if (Motor_Status == 0)
        ST_7567_printf(24+DISPALYOFFSETX, 0+DISPALYOFFSETY, "%3.d%%  %3.0f'", BatteryPer, T);
    else
    {
        run = !run;
        if (run == 0)
            ST_7567_printf(24+DISPALYOFFSETX, 0+DISPALYOFFSETY, "%3.d%%# %3.0f'", BatteryPer, T);
        else
            ST_7567_printf(24+DISPALYOFFSETX, 0+DISPALYOFFSETY, "%3.d%% #%3.0f'", BatteryPer, T);
    }
}
void display_Pressure(uint8_t unit)
{
    PT_KPA = get_Pt() - OFFSET_PT_KPA;
    if (PT_KPA < 5.0f)
        PT_KPA = 0.0f;
    PT_PSI = PT_KPA * 0.1450326;
    PT_BAR = PT_KPA * 0.01;
    switch (unit)
    {
    case 0:
        ST_7567_printf16(0+DISPALYOFFSETX, 1+DISPALYOFFSETY, "%6.0f kPa ", PT_KPA);
        break;
    case 1:
        ST_7567_printf16(0+DISPALYOFFSETX, 1+DISPALYOFFSETY, "%6.0f PSI ", PT_PSI);
        break;
    case 2:
        ST_7567_printf16(0+DISPALYOFFSETX, 1+DISPALYOFFSETY, "%6.1f Bar ", PT_BAR);
        break;
    default:
        break;
    }
    if (Motor_Status != 0)
    {
        switch (unit)
        {
        case 0:
            if (PT_KPA < default_set[Mode_Status])
            {
                Motor_ON();
            }
            else
            {
                Motor_Status = 0;
                Set_Status = 0;
                Motor_OFF();
            }
            break;
        case 1:
            if (PT_PSI < default_set[Mode_Status])
            {
                Motor_ON();
            }
            else
            {
                Motor_Status = 0;
                Set_Status = 0;
                Motor_OFF();
            }
            break;
        case 2:
            if (PT_BAR < default_set[Mode_Status])
            {
                Motor_ON();
            }
            else
            {
                Motor_Status = 0;
                Set_Status = 0;
                Motor_OFF();
            }
            break;
        default:
            break;
        }
        Motor_ON();
    }
    else
        Motor_OFF();
}
void display_Set(uint8_t unit)
{
    static uint8_t set = 0;
    switch (unit)
    {
    case 0:
        SET_PT_KPA = default_set[Mode_Status];
        SET_PT_PSI = SET_PT_KPA * 0.1450326;
        SET_PT_BAR = SET_PT_KPA * 0.01;
        if (set == 0)
            ST_7567_printf(0+DISPALYOFFSETX, 3+DISPALYOFFSETY, "Set:%6.0f kPa", SET_PT_KPA);
        else
            ST_7567_printf(0+DISPALYOFFSETX, 3+DISPALYOFFSETY, "   :%6.0f kPa", SET_PT_KPA);
        break;
    case 1:
        SET_PT_PSI = default_set[Mode_Status];
        SET_PT_KPA = SET_PT_PSI * 6.895;
        // SET_PT_PSI = SET_PT_KPA * 0.1450326;
        SET_PT_BAR = SET_PT_KPA * 0.01;
        if (set == 0)
            ST_7567_printf(0+DISPALYOFFSETX, 3+DISPALYOFFSETY, "Set:%6.0f PSI", SET_PT_PSI);
        else
            ST_7567_printf(0+DISPALYOFFSETX, 3+DISPALYOFFSETY, "   :%6.0f PSI", SET_PT_PSI);
        break;
    case 2:
        SET_PT_BAR = default_set[Mode_Status];
        SET_PT_KPA = SET_PT_BAR * 100;
        SET_PT_PSI = SET_PT_KPA * 0.1450326;
        // SET_PT_BAR = SET_PT_KPA * 0.01;
        if (set == 0)
            ST_7567_printf(0+DISPALYOFFSETX, 3+DISPALYOFFSETY, "Set:%6.1f Bar", SET_PT_BAR);
        else
            ST_7567_printf(0+DISPALYOFFSETX, 3+DISPALYOFFSETY, "   :%6.1f Bar", SET_PT_BAR);

        break;
    default:
        break;
    }
    if (Set_Status != 0)
        set = !set;
    else
        set = 0;
}
void display_Menu(uint8_t mode)
{
    uint8_t data[168] = {0};
    switch (mode)
    {
    case 0:
        for (int i = 0; i < 16; i++)
            data[2 + i] = menuico16_action[0][i];
        for (int i = 0; i < 16; i++)
            data[84 + 2 + i] = menuico16_action[1][i];
        for (int i = 0; i < 16; i++)
            data[22 + i] = menuico16[2][i];
        for (int i = 0; i < 16; i++)
            data[84 + 22 + i] = menuico16[3][i];
        for (int i = 0; i < 16; i++)
            data[42 + i] = menuico16[4][i];
        for (int i = 0; i < 16; i++)
            data[84 + 42 + i] = menuico16[5][i];
        for (int i = 0; i < 16; i++)
            data[62 + i] = menuico16[6][i];
        for (int i = 0; i < 16; i++)
            data[84 + 62 + i] = menuico16[7][i];
        break;
    case 1:
        for (int i = 0; i < 16; i++)
            data[2 + i] = menuico16[0][i];
        for (int i = 0; i < 16; i++)
            data[84 + 2 + i] = menuico16[1][i];
        for (int i = 0; i < 16; i++)
            data[22 + i] = menuico16_action[2][i];
        for (int i = 0; i < 16; i++)
            data[84 + 22 + i] = menuico16_action[3][i];
        for (int i = 0; i < 16; i++)
            data[42 + i] = menuico16[4][i];
        for (int i = 0; i < 16; i++)
            data[84 + 42 + i] = menuico16[5][i];
        for (int i = 0; i < 16; i++)
            data[62 + i] = menuico16[6][i];
        for (int i = 0; i < 16; i++)
            data[84 + 62 + i] = menuico16[7][i];
        break;
    case 2:
        for (int i = 0; i < 16; i++)
            data[2 + i] = menuico16[0][i];
        for (int i = 0; i < 16; i++)
            data[84 + 2 + i] = menuico16[1][i];
        for (int i = 0; i < 16; i++)
            data[22 + i] = menuico16[2][i];
        for (int i = 0; i < 16; i++)
            data[84 + 22 + i] = menuico16[3][i];
        for (int i = 0; i < 16; i++)
            data[42 + i] = menuico16_action[4][i];
        for (int i = 0; i < 16; i++)
            data[84 + 42 + i] = menuico16_action[5][i];
        for (int i = 0; i < 16; i++)
            data[62 + i] = menuico16[6][i];
        for (int i = 0; i < 16; i++)
            data[84 + 62 + i] = menuico16[7][i];
        break;
    case 3:
        for (int i = 0; i < 16; i++)
            data[2 + i] = menuico16[0][i];
        for (int i = 0; i < 16; i++)
            data[84 + 2 + i] = menuico16[1][i];
        for (int i = 0; i < 16; i++)
            data[22 + i] = menuico16[2][i];
        for (int i = 0; i < 16; i++)
            data[84 + 22 + i] = menuico16[3][i];
        for (int i = 0; i < 16; i++)
            data[42 + i] = menuico16[4][i];
        for (int i = 0; i < 16; i++)
            data[84 + 42 + i] = menuico16[5][i];
        for (int i = 0; i < 16; i++)
            data[62 + i] = menuico16_action[6][i];
        for (int i = 0; i < 16; i++)
            data[84 + 62 + i] = menuico16_action[7][i];
        break;
    default:
        break;
    }
    ST_7567_display16(0+DISPALYOFFSETX, 4+DISPALYOFFSETY, &data[0], 168);
}
void display(void *t)
{
    while (1)
    {
        if (get_powerstatic() != 0)
        {
            display_BatteryPer();
            display_Pressure(Unit_Status);
            display_Set(Unit_Status);
            display_Menu(Mode_Status);
        }
        else
        {
            Motor_OFF();
            LED_OFF();
            ST_7567_Poweroff();
        }
        if (Set_Status != 0)
        {
            Set_StatusT = 1;
        }
        if (Set_Status == 0 && Set_StatusT == 1 && (Set_StatusUintT != default_unit[0] || Set_StatusSetT != default_set[0]))
        {
            Set_StatusT = 0;
            ESP_ERROR_CHECK(NVS_Write(default_unit[0], default_set[0]));
            ESP_LOGI(TAG, "NVS_Write uintt=%d,sett=%f", default_unit[0], default_set[0]);
        }
        if (Set_Status == 0 && Set_StatusT == 0)
        {
            Set_StatusUintT = default_unit[0];
            Set_StatusSetT = default_set[0];
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
void Display_start()
{
    xTaskCreate(display, "display", 1024 * 100, NULL, 5, NULL);
}

void Init_Motor()
{
    gpio_reset_pin(IO_Motor);
    gpio_set_direction(IO_Motor, GPIO_MODE_OUTPUT);
    gpio_set_level(IO_Motor, 0);
}
void Motor_ON()
{
    gpio_set_level(IO_Motor, 1);
}
void Motor_OFF()
{
    gpio_set_level(IO_Motor, 0);
}
void Init_LED()
{
    gpio_reset_pin(LED_Out);
    gpio_set_direction(LED_Out, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_Out, 0);
}
void LED_ON()
{
    gpio_set_level(LED_Out, 1);
}
void LED_OFF()
{
    gpio_set_level(LED_Out, 0);
}

button_handle_t button_add;
button_handle_t button_sub;
button_handle_t button_power;
button_handle_t button_led;
static void button_add_click_event(void *in)
{
    ESP_LOGI(TAG, "button_add_click_event");

    if (Motor_Status == 0 && Set_Status == 0)
    {
        Mode_Status = (Mode_Status >= 3 ? 0 : Mode_Status + 1);
        ESP_LOGI(TAG, "Mode_Status=%d", Mode_Status);
        Unit_Status = default_unit[Mode_Status];
        switch (Unit_Status)
        {
        case 0:
            SET_PT_KPA = default_set[Mode_Status];
            break;
        case 1:
            SET_PT_PSI = default_set[Mode_Status];
            break;
        case 2:
            SET_PT_BAR = default_set[Mode_Status];
            break;
        default:
            break;
        }
    }
    else
    {
        switch (Unit_Status)
        {
        case 0:
            default_set[Mode_Status] += 5;
            if (default_set[Mode_Status] > 1000)
                default_set[Mode_Status] = 1000.0f;
            break;
        case 1:
            default_set[Mode_Status] += 1;
            if (default_set[Mode_Status] > 145)
                default_set[Mode_Status] = 145.0f;
            break;
        case 2:
            default_set[Mode_Status] += 0.1;
            if (default_set[Mode_Status] > 10.0)
                default_set[Mode_Status] = 10.0f;
            break;
        default:
            break;
        }
        ESP_LOGI(TAG, "default_set=%f", default_set[Mode_Status]);
    }
}
static void button_add_double_click_event(void *in)
{
    ESP_LOGI(TAG, "button_add_click_event");

    if (Motor_Status == 0 && Set_Status == 0)
    {
        return;
    }
    else
    {
        switch (Unit_Status)
        {
        case 0:
            default_set[Mode_Status] += 10;
            if (default_set[Mode_Status] > 1000)
                default_set[Mode_Status] = 1000.0f;
            break;
        case 1:
            default_set[Mode_Status] += 2;
            if (default_set[Mode_Status] > 145)
                default_set[Mode_Status] = 145.0f;
            break;
        case 2:
            default_set[Mode_Status] += 0.2;
            if (default_set[Mode_Status] > 10.0)
                default_set[Mode_Status] = 10.0f;
            break;
        default:
            break;
        }
        ESP_LOGI(TAG, "default_set=%f", default_set[Mode_Status]);
    }
}
static void button_add_long_hold_event(void *in)
{
    ESP_LOGI(TAG, "button_add_long_hold_event");
    if (Motor_Status == 0 && Set_Status == 0)
    {
        vTaskDelay(10);
        return;
    }
    else
    {
        switch (Unit_Status)
        {
        case 0:
            default_set[Mode_Status] += 5;
            if (default_set[Mode_Status] > 1000)
                default_set[Mode_Status] = 1000.0f;
            break;
        case 1:
            default_set[Mode_Status] += 1;
            if (default_set[Mode_Status] > 145)
                default_set[Mode_Status] = 145.0f;
            break;
        case 2:
            default_set[Mode_Status] += 0.1;
            if (default_set[Mode_Status] > 10.0)
                default_set[Mode_Status] = 10.0f;
            break;
        default:
            break;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(200));
}
static void button_sub_click_event(void *in)
{
    ESP_LOGI(TAG, "button_sub_click_event");
    if (Motor_Status == 0 && Set_Status == 0)
    {
        Mode_Status = (Mode_Status <= 0 ? 3 : Mode_Status - 1);
        ESP_LOGI(TAG, "Mode_Status=%d", Mode_Status);
        Unit_Status = default_unit[Mode_Status];
        switch (Unit_Status)
        {
        case 0:
            SET_PT_KPA = default_set[Mode_Status];
            break;
        case 1:
            SET_PT_PSI = default_set[Mode_Status];
            break;
        case 2:
            SET_PT_BAR = default_set[Mode_Status];
            break;
        default:
            break;
        }
    }
    else
    {

        switch (Unit_Status)
        {
        case 0:
            default_set[Mode_Status] -= 5;
            if (default_set[Mode_Status] < 10)
                default_set[Mode_Status] = 10.0f;
            break;
        case 1:
            default_set[Mode_Status] -= 1;
            if (default_set[Mode_Status] < 1)
                default_set[Mode_Status] = 1.0f;
            break;
        case 2:
            default_set[Mode_Status] -= 0.1;
            if (default_set[Mode_Status] < 0.1)
                default_set[Mode_Status] = 0.1f;
            break;
        default:
            break;
        }
        ESP_LOGI(TAG, "default_set=%f", default_set[Mode_Status]);
    }
}
static void button_sub_double_click_event(void *in)
{
    ESP_LOGI(TAG, "button_sub_click_event");
    if (Motor_Status == 0 && Set_Status == 0)
    {
        return;
    }
    else
    {

        switch (Unit_Status)
        {
        case 0:
            default_set[Mode_Status] -= 10;
            if (default_set[Mode_Status] < 10)
                default_set[Mode_Status] = 10.0f;
            break;
        case 1:
            default_set[Mode_Status] -= 2;
            if (default_set[Mode_Status] < 1)
                default_set[Mode_Status] = 1.0f;
            break;
        case 2:
            default_set[Mode_Status] -= 0.2;
            if (default_set[Mode_Status] < 0.1)
                default_set[Mode_Status] = 0.1f;
            break;
        default:
            break;
        }
        ESP_LOGI(TAG, "default_set=%f", default_set[Mode_Status]);
    }
}
static void button_sub_long_hold_event(void *in)
{
    ESP_LOGI(TAG, "button_sub_long_hold_event");
    if (Motor_Status == 0 && Set_Status == 0)
    {
        vTaskDelay(10);
        return;
    }
    else
    {
        switch (Unit_Status)
        {
        case 0:
            default_set[Mode_Status] -= 5;
            if (default_set[Mode_Status] < 10)
                default_set[Mode_Status] = 10.0f;
            break;
        case 1:
            default_set[Mode_Status] -= 1;
            if (default_set[Mode_Status] < 1)
                default_set[Mode_Status] = 1.0f;
            break;
        case 2:
            default_set[Mode_Status] -= 0.1;
            if (default_set[Mode_Status] < 0.1)
                default_set[Mode_Status] = 0.1f;
            break;
        default:
            break;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(200));
}
static void button_power_click_event(void *in)
{
    ESP_LOGI(TAG, "button_power_click_event");
    if (Motor_Status != 0)
    {
        Motor_Status = 0;
        Set_Status = 0;
    }
    else
    {
        Motor_Status++;
        Set_Status = 1;
    }
}
static void button_power_double_click_event(void *in)
{
    ESP_LOGI(TAG, "button_power_double_click_event");
    if (Motor_Status != 0)
        return;
    if (Set_Status != 0)
        Set_Status = 0;
    else
        Set_Status++;
}
static void button_power_thrice_click_event(void *in)
{
    ESP_LOGI(TAG, "button_power_thrice_click_event");

    if (Unit_Status >= 2)
        Unit_Status = 0;
    else
        Unit_Status++;
    default_unit[Mode_Status] = Unit_Status;
    switch (Unit_Status)
    {
    case 0:
        default_set[Mode_Status] = SET_PT_KPA;
        break;
    case 1:
        default_set[Mode_Status] = SET_PT_PSI;
        break;
    case 2:
        default_set[Mode_Status] = SET_PT_BAR;
        break;
    default:
        break;
    }
}
static void button_led_click_event(void *in)
{
    ESP_LOGI(TAG, "button_led_click_event");
    if (LED_Status == 0)
    {
        LED_Status = 1;
        LED_ON();
    }
    else
    {
        LED_Status = 0;
        LED_OFF();
    }
}
void init_mbutton()
{
    button_config_t gpio_btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = KEY_ADD,
            .active_level = 0,
        },
    };
    gpio_btn_cfg.gpio_button_config.gpio_num = KEY_ADD;
    button_add = button_create(&gpio_btn_cfg);
    if (NULL == button_add)
    {
        ESP_LOGE(TAG, "KEY_ADD Button create failed");
    }
    gpio_btn_cfg.gpio_button_config.gpio_num = KEY_SUB;
    button_sub = button_create(&gpio_btn_cfg);
    if (NULL == button_sub)
    {
        ESP_LOGE(TAG, "KEY_SUB Button create failed");
    }
    gpio_btn_cfg.gpio_button_config.gpio_num = KEY_POWER;
    button_power = button_create(&gpio_btn_cfg);
    if (NULL == button_power)
    {
        ESP_LOGE(TAG, "KEY_POWER Button create failed");
    }
    gpio_btn_cfg.gpio_button_config.gpio_num = LED_Button;
    button_led = button_create(&gpio_btn_cfg);
    if (NULL == button_led)
    {
        ESP_LOGE(TAG, "KEY_POWER Button create failed");
    }
    // 注册事件,
    button_register_cb(button_led, BUTTON_PRESS_DOWN, button_led_click_event);

    button_register_cb(button_add, BUTTON_PRESS_DOWN, button_add_click_event);
    // button_register_cb(button_add, BUTTON_DOUBLE_CLICK, button_add_double_click_event);
    button_register_cb(button_add, BUTTON_LONG_PRESS_HOLD, button_add_long_hold_event);

    button_register_cb(button_sub, BUTTON_PRESS_DOWN, button_sub_click_event);
    // button_register_cb(button_sub, BUTTON_DOUBLE_CLICK, button_sub_double_click_event);
    button_register_cb(button_sub, BUTTON_LONG_PRESS_HOLD, button_sub_long_hold_event);

    button_register_cb(button_power, BUTTON_SINGLE_CLICK, button_power_click_event);
    button_register_cb(button_power, BUTTON_DOUBLE_CLICK, button_power_double_click_event);
    button_register_cb(button_power, BUTTON_THRICE_CLICK, button_power_thrice_click_event);
}
