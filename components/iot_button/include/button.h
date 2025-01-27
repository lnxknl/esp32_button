// Copyright 2020 Espressif Systems (Shanghai) Co. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "button_adc.h"
#include "button_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (* button_cb_t)(void *);
typedef void *button_handle_t;
// @NOTE 
/**
 * @brief Button events
 *
 */
typedef enum {
    BUTTON_PRESS_DOWN = 0,
    BUTTON_PRESS_UP,
    BUTTON_PRESS_REPEAT,
    BUTTON_SINGLE_CLICK,
    BUTTON_DOUBLE_CLICK,
    BUTTON_THRICE_CLICK,
    BUTTON_LONG_PRESS_START,
    BUTTON_LONG_PRESS_HOLD,
    BUTTON_EVENT_MAX,
    BUTTON_NONE_PRESS,
} button_event_t;

/**
 * @brief Supported button type
 *
 */
typedef enum {
    BUTTON_TYPE_GPIO,
    BUTTON_TYPE_ADC,
} button_type_t;

/**
 * @brief Button configuration
 *
 */
typedef struct {
    button_type_t type;                           /**< button type, The corresponding button configuration must be filled */
    union {
        button_gpio_config_t gpio_button_config; /**< gpio button configuration */
        button_adc_config_t adc_button_config;   /**< adc button configuration */
    }; /**< button configuration */
} button_config_t;

typedef struct Button {
    uint16_t        ticks;
    uint8_t         repeat;
    button_event_t  event;
    uint8_t         state: 3;
    uint8_t         debounce_cnt: 3;
    uint8_t         active_level: 1;
    uint8_t         button_level: 1;
    uint8_t         (*hal_button_Level)(void *usr_data);
    void            *usr_data;
    button_type_t   type;
    button_cb_t     cb[BUTTON_EVENT_MAX];
    void            *cb_user_data;
    struct Button   *next;
} button_dev_t;

/**
 * @brief Create a button
 *
 * @param config pointer of button configuration, must corresponding the button type
 *
 * @return A handle to the created button, or NULL in case of error.
 */
button_handle_t button_create(const button_config_t *config);
//button_handle_t button_create(const button_config_t *config,uint16_t ticks,uint8_t state);
/**
 * @brief Delete a button
 *
 * @param btn_handle A button handle to delete
 *
 * @return
 *      - ESP_OK  Success
 *      - ESP_FAIL Failure
 */
esp_err_t button_delete(button_handle_t btn_handle);

/**
 * @brief Register the button event callback function.
 *
 * @param btn_handle A button handle to register
 * @param event Button event
 * @param cb Callback function.
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG   Arguments is invalid.
 */
esp_err_t button_register_cb(button_handle_t btn_handle, button_event_t event, button_cb_t cb);

/**
 * @brief Unregister the button event callback function.
 *
 * @param btn_handle A button handle to unregister
 * @param event Button event
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG   Arguments is invalid.
 */
esp_err_t button_unregister_cb(button_handle_t btn_handle, button_event_t event);

/**
 * @brief Get button event
 *
 * @param btn_handle Button handle
 *
 * @return Current button event. See button_event_t
 */
button_event_t button_get_event(button_handle_t btn_handle);

/**
 * @brief Get button repeat times
 *
 * @param btn_handle Button handle
 *
 * @return button pressed times. For example, double-click return 2, triple-click return 3, etc.
 */
uint8_t button_get_repeat(button_handle_t btn_handle);

#ifdef __cplusplus
}
#endif

#endif
