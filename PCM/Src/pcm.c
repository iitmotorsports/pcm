//
// Created by Noah Husby on 9/30/26.
//

#include "pcm.h"

#include "analog.h"
#include "can/pcm_can.h"
#include "can/pcm_can_heartbeat.h"
#include "main.h"
#include "pcm_status_led.h"
#include "pcm_usb.h"
#include "stm32h5xx_hal_gpio.h"
#include <stdint.h>

#define PCM_STATUS_LED_STACK_SIZE 512
#define PCM_STATUS_LED_PRIORITY 20
#define PCM_CAN_TX_STACK_SIZE 1024U
#define PCM_CAN_TX_PRIORITY 10U

#define PCM_CAN_HEARTBEAT_STACK_SIZE 512U
#define PCM_CAN_HEARTBEAT_PRIORITY 20U
// CAN runtime disabled until board integration and testing
#define PCM_CAN_RUNTIME_ENABLED 0
// Primary CAN bus confirmed as FDCAN1
#define PCM_CAN_HEARTBEAT_CONTROLLER PCM_CAN_CONTROLLER_1

static TX_THREAD pcm_can_tx_thread;
static uint8_t pcm_can_tx_stack[PCM_CAN_TX_STACK_SIZE];

static TX_THREAD pcm_can_heartbeat_thread;
static uint8_t pcm_can_heartbeat_stack[PCM_CAN_HEARTBEAT_STACK_SIZE];
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
    UINT status = tx_thread_create(
        &pcm_status_led_thread, "pcm_status_led", pcm_status_led_thread_entry,
        0, pcm_status_led_stack, PCM_STATUS_LED_STACK_SIZE,
        PCM_STATUS_LED_PRIORITY, PCM_STATUS_LED_PRIORITY, 1, TX_AUTO_START);
    if (status != TX_SUCCESS) {
        return TX_THREAD_ERROR;
    }
    // Create CAN message queue
    status = pcm_can_create(byte_pool);
    if (status != TX_SUCCESS) {
        return status;
    }

    // Create CAN transmit worker (not started yet)
    status = tx_thread_create(
        &pcm_can_tx_thread, "pcm_can_tx", pcm_can_thread_entry, 0UL,
        pcm_can_tx_stack, sizeof(pcm_can_tx_stack), PCM_CAN_TX_PRIORITY,
        PCM_CAN_TX_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START);

    if (status != TX_SUCCESS) {
        return status;
    }

    // Create heartbeat thread (not started yet)
    status = tx_thread_create(
        &pcm_can_heartbeat_thread, "pcm_can_heartbeat",
        pcm_can_heartbeat_thread_entry, PCM_CAN_HEARTBEAT_CONTROLLER,
        pcm_can_heartbeat_stack, sizeof(pcm_can_heartbeat_stack),
        PCM_CAN_HEARTBEAT_PRIORITY, PCM_CAN_HEARTBEAT_PRIORITY,
        TX_NO_TIME_SLICE, TX_DONT_START);

    if (status != TX_SUCCESS) {
        return status;
    }

    pcm_usb_init();
    if (pcm_usb_create(byte_pool) != TX_SUCCESS) {
        return TX_THREAD_ERROR;
    }

    pcm_analog_init();
    if (pcm_analog_create(byte_pool) != TX_SUCCESS) {
        return TX_THREAD_ERROR;
    }
#if PCM_CAN_RUNTIME_ENABLED

    // Initialize primary CAN (FDCAN1)
    pcm_can_init(PCM_CAN_CONTROLLER_1);

    // Start CAN transmit worker first
    status = tx_thread_resume(&pcm_can_tx_thread);

    if (status != TX_SUCCESS) {
        return status;
    }

    // Start heartbeat after CAN worker
    status = tx_thread_resume(&pcm_can_heartbeat_thread);

    if (status != TX_SUCCESS) {
        return status;
    }

#endif

    return status;
}