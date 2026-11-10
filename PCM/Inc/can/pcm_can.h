#ifndef PCM_CAN_H
#define PCM_CAN_H

#include "can/pcm_can_ids.h"
#include "tx_api.h"
#include <stdint.h>
#define PCM_CAN_MAX_DATA_BYTES 8U
#define PCM_CAN_QUEUE_DEPTH 8U
#define PCM_CAN_QUEUE_MESSAGE_WORDS TX_4_ULONG

typedef enum {
    PCM_CAN_CONTROLLER_1 = 0,
    PCM_CAN_CONTROLLER_2 = 1
} pcm_can_controller_t;

typedef struct {
    uint32_t id;
    uint8_t data[PCM_CAN_MAX_DATA_BYTES];
    uint8_t length;
    uint8_t controller;
    uint8_t is_extended;
    uint8_t reserved;
} pcm_can_message_t;

void pcm_can_init(pcm_can_controller_t controller);

UINT pcm_can_create(TX_BYTE_POOL *byte_pool);

UINT pcm_can_enqueue(const pcm_can_message_t *message);

VOID pcm_can_thread_entry(ULONG thread_input);
void pcm_can_rx(const uint8_t rx_data[], uint8_t rx_data_len, uint32_t rx_id);

#endif // PCM_CAN_H