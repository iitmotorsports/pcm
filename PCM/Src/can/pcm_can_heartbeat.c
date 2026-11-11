#include "can/pcm_can_heartbeat.h"
#include "can/pcm_can.h"

#define PCM_CAN_HEARTBEAT_INTERVAL_TICKS TX_TIMER_TICKS_PER_SECOND

static volatile uint32_t heartbeat_enqueue_failures = 0U;

VOID pcm_can_heartbeat_thread_entry(ULONG thread_input) {
    if (thread_input != PCM_CAN_CONTROLLER_1 &&
        thread_input != PCM_CAN_CONTROLLER_2) {
        return;
    }

    const pcm_can_message_t heartbeat = {
        .id = PCM_CAN_ID_HEARTBEAT,
        .data = {'P', 'C', 'M', ' ', 'O', 'K', 0x00, 0x00},
        .length = 8U,
        .controller = (uint8_t)thread_input,
        .is_extended = 0U};

    while (1) {
        if (pcm_can_enqueue(&heartbeat) != TX_SUCCESS) {
            heartbeat_enqueue_failures++;
        }

        tx_thread_sleep(PCM_CAN_HEARTBEAT_INTERVAL_TICKS);
    }
}