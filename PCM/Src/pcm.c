//
// Created by Noah Husby on 9/30/26.
//

#include "pcm.h"

#include <stdint.h>

#include "main.h"
#include "pcm_status_led.h"
#include "stm32h5xx_hal_gpio.h"

#define PCM_STATUS_LED_STACK_SIZE 512
#define PCM_STATUS_LED_PRIORITY 20

static TX_THREAD pcm_status_led_thread;
static uint8_t pcm_status_led_stack[PCM_STATUS_LED_STACK_SIZE];

void pcm_init(void) {
    uint16_t i = 0;
    while (i < 5000) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        HAL_Delay(50);
        i += 50;
    }
}

UINT pcm_threads_create(TX_BYTE_POOL *byte_pool) {
    UINT status = tx_thread_create(&pcm_status_led_thread, "pcm_status_led", pcm_status_led_thread_entry, 0, pcm_status_led_stack, PCM_STATUS_LED_STACK_SIZE, PCM_STATUS_LED_PRIORITY, PCM_STATUS_LED_PRIORITY, 1, TX_AUTO_START);
    if (status != TX_SUCCESS) {
        return TX_THREAD_ERROR;
    }
    return status;
}