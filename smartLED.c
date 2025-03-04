/* BEGIN Header */
/**
 ******************************************************************************
 * \file            smartLED.c
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

/* Includes ------------------------------------------------------------------*/

#include <stdlib.h>
#include "smartLED.h"
#include "string.h"

/* Macros --------------------------------------------------------------------*/

/* PWM frequency in kHz */
#ifndef SMARTLED_PWM_FREQ
#define SMARTLED_PWM_FREQ 800
#endif /* SMARTLED_PWM_FREQ */

/* Private Functions ---------------------------------------------------------*/
static smartLED_retStatus_t smartLED_fillDMABuffer(smartLED_t* smartled, uint16_t item, uint32_t startingIdx) {
    if (item >= smartled->size) {
        return SMARTLED_ERROR;
    }

    uint8_t r, g, b;
    uint32_t itemIdx = item * smartled->type;

    r = (uint8_t)(((uint32_t)smartled->_colorsData[itemIdx] * (uint32_t)smartled->_brightness) / (uint32_t)0xFF);
    g = (uint8_t)(((uint32_t)smartled->_colorsData[itemIdx + 1] * (uint32_t)smartled->_brightness) / (uint32_t)0xFF);
    b = (uint8_t)(((uint32_t)smartled->_colorsData[itemIdx + 2] * (uint32_t)smartled->_brightness) / (uint32_t)0xFF);

    for (uint32_t ii = startingIdx, jj = 7; ii < (8 + startingIdx); ii++, jj--) {
        smartled->_dmaBuffer[ii] = (g & (1 << jj)) ? smartled->_pulseHigh : smartled->_pulseLow;
        smartled->_dmaBuffer[ii + 8] = (r & (1 << jj)) ? smartled->_pulseHigh : smartled->_pulseLow;
        smartled->_dmaBuffer[ii + 16] = (b & (1 << jj)) ? smartled->_pulseHigh : smartled->_pulseLow;
    }
    return SMARTLED_SUCCESS;
}

/* Functions -----------------------------------------------------------------*/

smartLED_retStatus_t smartLED_init(smartLED_t* smartled) {
    /* Check chip type */
    if ((smartled->chip != WS2811) && (smartled->chip != WS2812B)) {
        return SMARTLED_ERROR;
    }

    /* Check led type */
    if ((smartled->type != SMARTLED_RGB) && (smartled->type != SMARTLED_RGBW)) {
        return SMARTLED_ERROR;
    }

    /* Check timer type */
    if ((smartled->timType != SMARTLED_TIMER_NORMAL) && (smartled->timType != SMARTLED_TIMER_EXTENDED)) {
        return SMARTLED_ERROR;
    }

    /* Check number of LEDs per each IRQ */
    if (smartled->LEDperIRQ < 1) {
        return SMARTLED_ERROR;
    }

    /* Check that timer has been configured */
    if (!smartled->htim->Instance->ARR) {
        return SMARTLED_ERROR;
    }

    smartled->_brightness = 0xFF;
    smartled->_pulseLow = (uint8_t)((float)smartled->htim->Instance->ARR * 0.33f);
    smartled->_pulseHigh = (uint8_t)((float)smartled->htim->Instance->ARR * 0.65f);
    smartled->_LEDBits = smartled->type * 8;

    /* Set the right amount of empty LED blocks needed at the beginning to initiate transfer */
    if (smartled->chip == WS2811) {
        smartled->_resetBlocks = 280e-3 * SMARTLED_PWM_FREQ / smartled->_LEDBits + 2;
    } else {
        smartled->_resetBlocks = 50e-3 * SMARTLED_PWM_FREQ / smartled->_LEDBits + 2;
    }

    smartled->_colorsData = calloc(smartled->type * smartled->size, sizeof(uint8_t));
    if (smartled->_colorsData == NULL) {
        return SMARTLED_ERROR;
    }

    smartled->_dmaBuffer = calloc(2 * smartled->LEDperIRQ * smartled->_LEDBits, sizeof(uint8_t));
    if (smartled->_dmaBuffer == NULL) {
        return SMARTLED_ERROR;
    }

    return SMARTLED_SUCCESS;
}

smartLED_retStatus_t smartLED_initStatic(smartLED_t* smartled, uint8_t* data, uint8_t* DMABuffer) {
    /* Check chip type */
    if ((smartled->chip != WS2811) && (smartled->chip != WS2812B)) {
        return SMARTLED_ERROR;
    }

    /* Check led type */
    if ((smartled->type != SMARTLED_RGB) && (smartled->type != SMARTLED_RGBW)) {
        return SMARTLED_ERROR;
    }

    /* Check timer type */
    if ((smartled->timType != SMARTLED_TIMER_NORMAL) && (smartled->timType != SMARTLED_TIMER_EXTENDED)) {
        return SMARTLED_ERROR;
    }

    /* Check number of LEDs per each IRQ */
    if (smartled->LEDperIRQ < 1) {
        return SMARTLED_ERROR;
    }

    /* Check that timer has been configured */
    if (!smartled->htim->Instance->ARR) {
        return SMARTLED_ERROR;
    }

    smartled->_brightness = 0xFF;
    smartled->_pulseLow = (uint8_t)((float)smartled->htim->Instance->ARR * 0.33f);
    smartled->_pulseHigh = (uint8_t)((float)smartled->htim->Instance->ARR * 0.65f);
    smartled->_LEDBits = smartled->type * 8;

    /* Set the right amount of empty LED blocks needed between two consecutive transfers */
    if (smartled->chip == WS2811) {
        smartled->_resetBlocks = 280e-3 * SMARTLED_PWM_FREQ / smartled->_LEDBits + 2;
    } else {
        smartled->_resetBlocks = 50e-3 * SMARTLED_PWM_FREQ / smartled->_LEDBits + 2;
    }
    smartled->_colorsData = data;
    smartled->_dmaBuffer = DMABuffer;

    return SMARTLED_SUCCESS;
}

smartLED_retStatus_t smartLED_startTransfer(smartLED_t* smartled) {
    if (smartled->_updating) {
        return SMARTLED_ERROR;
    }

    /* Set initial values */
    smartled->_updating = 1;
    smartled->_cyclesCnt = smartled->LEDperIRQ;

    /* Fill the entire DMA buffer with the first set of elements */
    memset(smartled->_dmaBuffer, 0x00, sizeof(uint8_t) * 2u * smartled->LEDperIRQ * smartled->_LEDBits);
    for (uint16_t ii = 0; ii < 2 * smartled->LEDperIRQ; ii++) {
        smartLED_fillDMABuffer(smartled, ii, ii * smartled->_LEDBits);
    }

    /* Start Transfer */
    if (smartled->timType == SMARTLED_TIMER_NORMAL) {
        HAL_TIM_PWM_Start_DMA(smartled->htim, smartled->timChannel, (uint32_t*)smartled->_dmaBuffer, 2 * smartled->LEDperIRQ * smartled->_LEDBits);
    } else {
        HAL_TIMEx_PWMN_Start_DMA(smartled->htim, smartled->timChannel, (uint32_t*)smartled->_dmaBuffer, 2 * smartled->LEDperIRQ * smartled->_LEDBits);
    }
    return SMARTLED_SUCCESS;
}

smartLED_retStatus_t smartLED_updateTransfer(smartLED_t* smartled, smartLEDIRQType_t PWM_IRQ) {
    if (!smartled->_updating) {
        return SMARTLED_ERROR;
    }
    /* When interrupt is triggered, DMA already started transfer of the next half of the buffer, 
     * so the code rewrites the one that was just transmitted */
    uint32_t DMABuffHalfCpltLen = (uint32_t)(smartled->LEDperIRQ * smartled->_LEDBits);

    /* Interrupts are triggered (TC or HT) when DMA transfers `smartled->LEDperIRQ` led cycles of data elements
     * The increment of _cyclesCnt is anticipated compared to the actual transfer */
    smartled->_cyclesCnt += smartled->LEDperIRQ;

    if (smartled->_cyclesCnt < smartled->size) {
        uint16_t next_led = smartled->_cyclesCnt;
        uint8_t counter = 0;
        uint32_t startIdx = PWM_IRQ * DMABuffHalfCpltLen;

        /* Fill buffer with led data, paying attention to alignment with post reset blocks*/
        for (; counter < smartled->LEDperIRQ && next_led < smartled->size; counter++, next_led++) {
            smartLED_fillDMABuffer(smartled, next_led, startIdx + counter * smartled->_LEDBits);
        }
        if (counter < smartled->LEDperIRQ) {
            memset(&(smartled->_dmaBuffer[startIdx + counter * smartled->_LEDBits]), 0x00,
                   sizeof(uint8_t) * (smartled->LEDperIRQ - counter) * smartled->type * 8u);
        }
    } else if (smartled->_cyclesCnt < (uint32_t)(smartled->_resetBlocks + smartled->size + smartled->LEDperIRQ)) {
        /* Reset array to all zeros after transfer is complete 
         * Reset happens just once, not to waste CPU resources 
         * It assumes to send at least another _resetBlocks number of empty LED blocks after transfer is completed */
        if (smartled->_cyclesCnt < (smartled->size + 2u * smartled->LEDperIRQ)) {
            memset(&(smartled->_dmaBuffer[PWM_IRQ * DMABuffHalfCpltLen]), 0x00, sizeof(uint8_t) * DMABuffHalfCpltLen);
        }
    } else {
        /* Stop PWM */
        if (smartled->timType == SMARTLED_TIMER_NORMAL) {
            HAL_TIM_PWM_Stop_DMA(smartled->htim, smartled->timChannel);
        } else {
            HAL_TIMEx_PWMN_Stop_DMA(smartled->htim, smartled->timChannel);
        }
        smartled->_updating = 0;
    }
    return SMARTLED_SUCCESS;
}