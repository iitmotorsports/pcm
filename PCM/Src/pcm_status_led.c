//
// Created by Noah Husby on 9/30/26.
//

#include "pcm_status_led.h"

#include "main.h"
#include "stm32h5xx_hal_gpio.h"
#include "tx_api.h"

VOID pcm_status_led_thread_entry(ULONG thread_input) {
    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2);
    }
}