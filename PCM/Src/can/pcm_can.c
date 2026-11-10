
#include "can/pcm_can.h"
#include "main.h"

// CAN peripheral handles
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;

#define PCM_CAN_TX_MAX_ATTEMPTS 20U

// Ensure each queue message occupies exactly four ThreadX words
_Static_assert(sizeof(pcm_can_message_t) == 4U * sizeof(ULONG),
               "CAN message must occupy four ThreadX words");

// ThreadX CAN queue
static TX_QUEUE can_message_queue;
static VOID *can_queue_storage = TX_NULL;
static uint8_t can_queue_ready = 0U;

// CAN diagnostics
static volatile uint32_t can_tx_failures = 0U;
static volatile uint32_t can_rx_frames = 0U;
static volatile uint32_t can_rx_errors = 0U;
static volatile uint32_t can_rx_last_id = 0U;

// ---------------------------------------------------------
// Create CAN Message Queue
// ---------------------------------------------------------

UINT pcm_can_create(TX_BYTE_POOL *byte_pool) {
    if (byte_pool == TX_NULL) {
        return TX_PTR_ERROR;
    }

    if (can_queue_ready) {
        return TX_SUCCESS;
    }

    UINT status = tx_byte_allocate(
        byte_pool, &can_queue_storage,
        PCM_CAN_QUEUE_DEPTH * sizeof(pcm_can_message_t), TX_NO_WAIT);

    if (status != TX_SUCCESS) {
        return status;
    }

    status = tx_queue_create(&can_message_queue, "PCM CAN",
                             PCM_CAN_QUEUE_MESSAGE_WORDS, can_queue_storage,
                             PCM_CAN_QUEUE_DEPTH * sizeof(pcm_can_message_t));

    if (status != TX_SUCCESS) {
        tx_byte_release(can_queue_storage);
        can_queue_storage = TX_NULL;
        return status;
    }

    can_queue_ready = 1U;

    return TX_SUCCESS;
}

// ---------------------------------------------------------
// Add CAN Message to ThreadX Queue
// ---------------------------------------------------------

UINT pcm_can_enqueue(const pcm_can_message_t *message) {
    if (message == TX_NULL) {
        return TX_PTR_ERROR;
    }

    if (!can_queue_ready) {
        return TX_QUEUE_ERROR;
    }

    if (message->length > PCM_CAN_MAX_DATA_BYTES) {
        return TX_SIZE_ERROR;
    }

    if (message->controller > PCM_CAN_CONTROLLER_2 ||
        message->is_extended > 1U) {
        return TX_OPTION_ERROR;
    }

    if (message->id > (message->is_extended ? 0x1FFFFFFFU : 0x7FFU)) {
        return TX_OPTION_ERROR;
    }

    return tx_queue_send(&can_message_queue, (VOID *)message, TX_NO_WAIT);
}

// ---------------------------------------------------------
// CAN Transmit Worker Thread
// ---------------------------------------------------------

VOID pcm_can_thread_entry(ULONG thread_input) {
    (void)thread_input;

    pcm_can_message_t message;

    while (1) {
        if (tx_queue_receive(&can_message_queue, &message, TX_WAIT_FOREVER) !=
            TX_SUCCESS) {
            continue;
        }

        FDCAN_HandleTypeDef *hfdcan =
            (message.controller == PCM_CAN_CONTROLLER_1) ? &hfdcan1 : &hfdcan2;

        FDCAN_TxHeaderTypeDef header = {0};

        header.Identifier = message.id;
        header.IdType =
            message.is_extended ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;

        header.TxFrameType = FDCAN_DATA_FRAME;
        header.DataLength = message.length;
        header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
        header.BitRateSwitch = FDCAN_BRS_OFF;
        header.FDFormat = FDCAN_CLASSIC_CAN;
        header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
        header.MessageMarker = 0U;

        HAL_StatusTypeDef status = HAL_ERROR;

        for (uint32_t attempt = 0; attempt < PCM_CAN_TX_MAX_ATTEMPTS;
             attempt++) {
            if (HAL_FDCAN_GetTxFifoFreeLevel(hfdcan) > 0U) {
                status = HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &header,
                                                       message.data);

                if (status == HAL_OK) {
                    break;
                }
            }

            tx_thread_sleep(1U);
        }

        if (status != HAL_OK) {
            can_tx_failures++;
        }
    }
}

// ---------------------------------------------------------
// Initialize CAN Peripheral
// ---------------------------------------------------------

void pcm_can_init(pcm_can_controller_t controller) {
    FDCAN_HandleTypeDef *hfdcan;

    switch (controller) {
    case PCM_CAN_CONTROLLER_1:
        hfdcan = &hfdcan1;
        break;

    case PCM_CAN_CONTROLLER_2:
        hfdcan = &hfdcan2;
        break;

    default:
        Error_Handler();
        return;
    }

    // Configure FDCAN1 acceptance filter
    // Accept FAN_SEND (0x002) into RX FIFO0
    if (controller == PCM_CAN_CONTROLLER_1) {
        FDCAN_FilterTypeDef filter = {0};

        filter.IdType = FDCAN_STANDARD_ID;
        filter.FilterIndex = 0U;
        filter.FilterType = FDCAN_FILTER_MASK;
        filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
        filter.FilterID1 = PCM_CAN_ID_FAN_SEND;
        filter.FilterID2 = 0x7FFU;

        if (HAL_FDCAN_ConfigFilter(hfdcan, &filter) != HAL_OK) {
            Error_Handler();
            return;
        }
    }

    // Reject all non-matching CAN messages
    if (HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT,
                                     FDCAN_REJECT_REMOTE,
                                     FDCAN_REJECT_REMOTE) != HAL_OK) {
        Error_Handler();
        return;
    }

    // Start selected CAN peripheral
    if (HAL_FDCAN_Start(hfdcan) != HAL_OK) {
        Error_Handler();
        return;
    }

    // Enable RX FIFO0 notifications on FDCAN1
    if (controller == PCM_CAN_CONTROLLER_1) {
        if (HAL_FDCAN_ActivateNotification(
                hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0U) != HAL_OK) {
            Error_Handler();
            return;
        }
    }
}

// ---------------------------------------------------------
// CAN Receive Handler
// ---------------------------------------------------------

void pcm_can_rx(const uint8_t rx_data[], uint8_t rx_data_len, uint32_t rx_id) {
    // Message-specific processing can be added here.
    // For now, record received message diagnostics.

    (void)rx_data;
    (void)rx_data_len;

    can_rx_last_id = rx_id;
    can_rx_frames++;
}

// ---------------------------------------------------------
// FDCAN1 RX FIFO0 Interrupt Callback
// ---------------------------------------------------------

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan,
                               uint32_t RxFifo0ITs) {
    if (hfdcan != &hfdcan1 ||
        (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U) {
        return;
    }

    while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0U) {
        FDCAN_RxHeaderTypeDef rx_header = {0};
        uint8_t rx_data[PCM_CAN_MAX_DATA_BYTES] = {0};

        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header,
                                   rx_data) != HAL_OK) {
            can_rx_errors++;
            break;
        }

        // Only accept standard Classic CAN data frames
        if (rx_header.IdType != FDCAN_STANDARD_ID ||
            rx_header.RxFrameType != FDCAN_DATA_FRAME ||
            rx_header.FDFormat != FDCAN_CLASSIC_CAN ||
            rx_header.DataLength > FDCAN_DLC_BYTES_8) {
            continue;
        }

        pcm_can_rx(rx_data, (uint8_t)rx_header.DataLength,
                   rx_header.Identifier);
    }
}
