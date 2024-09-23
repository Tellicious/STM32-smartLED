/* BEGIN Header */
/**
 ******************************************************************************
 * \file            smartLED.h
 * \author          Andrea Vivani
 * \brief           Control of WS2811 and WS2812B LED strips
 ******************************************************************************
 * \copyright
 *
 * Copyright 2024 Andrea Vivani
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 ******************************************************************************
 */
/* END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SMARTLED_H__
#define __SMARTLED_H__

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/

#include <stdint.h>
#include "commonTypes.h"
#include "tim.h"

/* Typedefs ------------------------------------------------------------------*/

/**
 * LED chip
 */
typedef enum {
    WS2811 = 0,
    WS2812B = 1,
} smartLEDChip_t;

/**
 * LED type
 */
typedef enum {
    SMARTLED_RGB = 3,
    SMARTLED_RGBW = 4,
} smartLEDType_t;

/**
 * LED color
 */
typedef enum {
    SMARTLED_RED = 0,
    SMARTLED_GREEN = 1,
    SMARTLED_BLUE = 2,
    SMARTLED_WHITE = 3,
} smartLEDColor_t;

/**
 * PWM Timer type
 */
typedef enum {
    SMARTLED_TIMER_NORMAL = 0,
    SMARTLED_TIMER_EXTENDED = 1,
} smartLEDTimerType_t;

/**
 * PWM Interrupt type
 */
typedef enum {
    SMARTLED_IRQ_HALFCPLT = 0,
    SMARTLED_IRQ_FINISHED = 1,
} smartLEDIRQType_t;

/*
* SMARTLED return status
*/
typedef enum { SMARTLED_SUCCESS = 0, SMARTLED_ERROR = 1, SMARTLED_TIMEOUT = 2 } smartLED_retStatus_t;

/**
 * LED struct
 */
typedef struct {
    /* Public */
    smartLEDChip_t chip;
    smartLEDType_t type;
    uint16_t size;
    TIM_HandleTypeDef* htim;
    smartLEDTimerType_t timType;
    uint32_t timChannel;
    uint8_t LEDperIRQ; // number of LEDs to be updated per each PWM IRQ
    /* Private */
    uint8_t* _colorsData;
    uint16_t* _dmaBuffer;
    uint8_t _brightness;
    uint8_t _updating;
    uint32_t _cyclesCnt;
    uint16_t _pulseLow, _pulseHigh; // length of 0 and 1 PWM pulses
    uint8_t _LEDBits;               // bits to define LED color (8 * num of colors)
    uint8_t
        _resetBlocks; // number of 1-led-transmission-time" blocks to send logical `0` to the bus, indicating reset before data transmission starts
} smartLED_t;

/* Function prototypes -------------------------------------------------------*/

/**
 * \brief           Init smart LED structure with dynamic memory allocation
 *
 * \param[in]       smartled: pointer to smart LED object
 *
 * \return          SMARTLED_SUCCESS if parameters are configured correctly, SMARTLED_ERROR otherwise
 */
smartLED_retStatus_t smartLED_init(smartLED_t* smartled);

/**
 * \brief           Init smart LED structure with static memory allocation
 *
 * \param[in]       smartled: pointer to smart LED object
 * \param[in]       data: pointer to LED data array of size nColors * nLEDs
 * \param[in]       BMABuffer: pointer to DMA buffer array of size 2 * LEDperIRQ * nColors * 8
 *
 * \return          SMARTLED_SUCCESS if parameters are configured correctly, SMARTLED_ERROR otherwise
 */
smartLED_retStatus_t smartLED_initStatic(smartLED_t* smartled, uint8_t* data, uint16_t* DMABuffer);

/**
 * \brief           Set smart LED brightness
 *
 * \param[in]       smartled: pointer to smart LED object
 * \param[in]       brightness: value of LED strip brightness, from 0 to 255
 */
#define smartLED_setBrightness(smartled, brightness) (smartled)->_brightness = (brightness) & 0xFF

/**
 * \brief           Increase smart LED brightness
 *
 * \param[in]       smartled: pointer to smart LED object
 */
#define smartLED_increaseBrightness(smartled)        (smartled)->_brightness++

/**
 * \brief           Decrease smart LED brightness
 *
 * \param[in]       smartled: pointer to smart LED object
 */
#define smartLED_decreaseBrightness(smartled)        (smartled)->_brightness--

/**
 * \brief           Set RGB colors of smart LED item
 *
 * \param[in]       smartled: pointer to smart LED object
 * \param[in]       item: LED number to be changed
 * \param[in]       red: value of red color, from 0 to 255
 * \param[in]       green: value of green color, from 0 to 255
 * \param[in]       blue: value of blue color, from 0 to 255
 */
#define smartLED_updateRGBColors(smartled, item, red, green, blue)                                                     \
    do {                                                                                                               \
        (smartled)->_colorsData[(item) * (smartled)->type] = (red) & 0xFF;                                             \
        (smartled)->_colorsData[(item) * (smartled)->type + 1u] = (green) & 0xFF;                                      \
        (smartled)->_colorsData[(item) * (smartled)->type + 2u] = (blue) & 0xFF;                                       \
    } while (0)

/**
 * \brief           Set RGBW colors of smart LED item
 *
 * \param[in]       smartled: pointer to smart LED object
 * \param[in]       item: LED number to be changed
 * \param[in]       red: value of red color, from 0 to 255
 * \param[in]       green: value of green color, from 0 to 255
 * \param[in]       blue: value of blue color, from 0 to 255
 * \param[in]       white: value of white color, from 0 to 255
 */
#define smartLED_updateRGBWColors(smartled, item, red, green, blue)                                                    \
    do {                                                                                                               \
        if ((smartled)->type == SMARTLED_RGBW) {                                                                       \
            (smartled)->_colorsData[(item) * 4u] = (red) & 0xFF;                                                       \
            (smartled)->_colorsData[(item) * 4u + 1u] = (green) & 0xFF;                                                \
            (smartled)->_colorsData[(item) * 4u + 2u] = (blue) & 0xFF;                                                 \
            (smartled)->_colorsData[(item) * 4u + 3u] = (white) & 0xFF;                                                \
        }                                                                                                              \
    } while (0)

/**
 * \brief           Set specific color of smart LED item
 *
 * \param[in]       smartled: pointer to smart LED object
 * \param[in]       item: LED number to be changed
 * \param[in]       color: color to be changed, SMARTLED_RED, SMARTLED_GREEN, SMARTLED_BLUE, SMARTLED_WHITE
 * \param[in]       value: value of color, from 0 to 255
 */
#define smartLED_updateColor(smartled, item, color, value)                                                             \
    (smartled)->_colorsData[(item) * (smartled)->type + (color)] = (value) & 0xFF

/**
 * \brief           Set RGB colors of all smart LED items
 *
 * \param[in]       smartled: pointer to smart LED object
 * \param[in]       red: value of red color, from 0 to 255
 * \param[in]       green: value of green color, from 0 to 255
 * \param[in]       blue: value of blue color, from 0 to 255
 */
#define smartLED_updateAllRGBColors(smartled, red, green, blue)                                                        \
    for (uint16_t ii = 0; ii < (smartled)->size; ii++) {                                                               \
        (smartled)->_colorsData[ii * (smartled)->type] = (red) & 0xFF;                                                 \
        (smartled)->_colorsData[ii * (smartled)->type + 1u] = (green) & 0xFF;                                          \
        (smartled)->_colorsData[ii * (smartled)->type + 2u] = (blue) & 0xFF;                                           \
    }

/**
 * \brief           Set RGBW colors of all smart LED items
 *
 * \param[in]       smartled: pointer to smart LED object
 * \param[in]       red: value of red color, from 0 to 255
 * \param[in]       green: value of green color, from 0 to 255
 * \param[in]       blue: value of blue color, from 0 to 255
 * \param[in]       white: value of white color, from 0 to 255
 */
#define smartLED_updateAllRGBWColors(smartled, red, green, blue, white)                                                \
    if ((smartled)->type == SMARTLED_RGBW) {                                                                           \
        for (uint16_t ii = 0; ii < (smartled)->size; ii++) {                                                           \
            (smartled)->_colorsData[ii * 4u] = (red) & 0xFF;                                                           \
            (smartled)->_colorsData[ii * 4u + 1u] = (green) & 0xFF;                                                    \
            (smartled)->_colorsData[ii * 4u + 2u] = (blue) & 0xFF;                                                     \
            (smartled)->_colorsData[ii * 4u + 3u] = (white) & 0xFF;                                                    \
        }                                                                                                              \
    }

/**
 * \brief           Start data transfer to smart LED
 *
 * \param[in]       smartled: pointer to smart LED object
 * 
 * \return          SMARTLED_SUCCESS if transfer can be initiated, SMARTLED_ERROR otherwise
 */
smartLED_retStatus_t smartLED_startTransfer(smartLED_t* smartled);

/**
 * \brief           Update DMA buffer according to defined LED sequence
 * \attention       Function to be called by HAL_TIM_PWM_PulseFinishedHalfCpltCallback() and HAL_TIM_PWM_PulseFinishedCallback()
 *
 * \param[in]       smartled: pointer to smart LED object
 * \param[in]       PWM_IRQ: type of interrupt that is calling the function, either SMARTLED_IRQ_HALFCPLT or SMARTLED_IRQ_FINISHED
 * 
 * \return          SMARTLED_SUCCESS if data can be updated succesfully, SMARTLED_ERROR otherwise
 */
smartLED_retStatus_t smartLED_updateTransfer(smartLED_t* smartled, smartLEDIRQType_t PWM_IRQ);

#ifdef __cplusplus
}
#endif

#endif /* __SMARTLED_H__ */
