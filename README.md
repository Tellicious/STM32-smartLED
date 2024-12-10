# WS2811 and WS21812B smart LED driver for STM32

### CubeMX setup:
- Set PWM timer with 800 kHz frequency using only the autoReload register, with prescaler = 0. Overclocking up to 1.2 MHz frequency can be achieved, but results may vary
- Enable auto-reload preload
- Set DMA Memory->Peripheral in circular mode, with peripheral data width `Word` and memory data width `Byte` and with "Increment address" selected for memory

### Example usage:
- Initialization:
    ```cpp
    /* initialize LED strip */
    smartLED_t LEDstrip;
    LEDstrip.chip = WS2812B;
    LEDstrip.type = SMARTLED_RGB;
    LEDstrip.size = 30;
    LEDstrip.htim = &htim1;
    LEDstrip.timType = SMARTLED_TIMER_EXTENDED;
    LEDstrip.timChannel = TIM_CHANNEL_3;
    LEDstrip.LEDperIRQ = 8;

    /* LED Strip initialization, after MX_TIM1_Init();*/
    if (smartLED_init(&LEDstrip) != SMARTLED_SUCCESS) {
        while (1);
    }
    ```
- Interrupt Handlers:
    ```cpp
    void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef* htim) {
        /* Process LED strip */
        if (htim->Instance == htim1.Instance) {
            smartLED_updateTransfer(&LEDstrip, SMARTLED_IRQ_HALFCPLT);
        }
    }

    void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef* htim) {
        /* Process LED strip */
        if (htim->Instance == htim1.Instance) {
            smartLED_updateTransfer(&LEDstrip, SMARTLED_IRQ_FINISHED);
        }
    }
    ```
- Transmit data:
    ```cpp
    smartLED_updateAllRGBColors(&LEDstrip, 0, 0, 0);
    while (smartLED_startTransfer(&LEDstrip) != SMARTLED_SUCCESS) {
        HAL_Delay(10);
    }
    ```

### Configurable parameters

| Parameter name | Description                                                        | Values                                               |
| -------------- | ------------------------------------------------------------------ | ---------------------------------------------------- |
| chip           | Type of chip to be controlled                                      | `WS2811` or `WS2812B`                                |
| type           | Type of LED device: RGB or RGBW                                    | `SMARTLED_RGB` or `SMARTLED_RGBW`                    |
| size           | Number of LEDs                                                     | #                                                    |
| htim           | Pointer to PWM timer handle                                        | &htim1                                               |
| timType        | Type of PWM timer: normal or extended (N channels)                 | `SMARTLED_TIMER_NORMAL` or `SMARTLED_TIMER_EXTENDED` |
| timChannel     | Timer channel number                                               | TIM_CHANNEL_3                                        |
| LEDperIRQ      | Number of LED blocks to be sent between two consecutive interrupts | Suggested from 5 to 10                               |
