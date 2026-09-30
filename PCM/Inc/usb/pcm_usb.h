//
// Created by Noah Husby on 9/30/26.
//

#ifndef PCM_USB_H
#define PCM_USB_H

#define PCM_USB_THREAD_STACK_SIZE 2048
#define PCM_USB_THREAD_PRIORITY 9

#define PCM_USB_MAX_PACKET_SIZE 64
#define PCM_USB_TX_BUFFER_SIZE 256
#define PCM_USB_TX_QUEUE_DEPTH 8
#define PCM_USB_QUEUE_MESSAGE_WORDS TX_1_ULONG
#include <stdbool.h>

#include "tx_api.h"
#include "tx_port.h"
#include "ux_device_class_cdc_acm.h"

typedef enum
{
    PCM_USB_ENDPOINT_CONSOLE = 0,
    PCM_USB_ENDPOINT_VENDOR,
} pcm_usb_endpoint_t;

typedef struct
{
    pcm_usb_endpoint_t endpoint;
    ULONG length;
    UCHAR data[PCM_USB_TX_BUFFER_SIZE];
    bool allocated;
} pcm_usb_packet_t;

typedef struct
{
    bool connected;
    bool console_session_started;
    UX_SLAVE_CLASS_CDC_ACM* cdc;

    TX_THREAD tx_thread;
    TX_THREAD rx_thread;

    TX_QUEUE tx_queue;

    TX_MUTEX tx_packet_mutex;

    VOID* tx_stack;
    VOID* rx_stack;

    VOID* tx_queue_memory;

    pcm_usb_packet_t tx_packets[PCM_USB_TX_QUEUE_DEPTH];
} pcm_usb_t;

void pcm_usb_init(void);

UINT pcm_usb_create(TX_BYTE_POOL* byte_pool);

bool pcm_usb_connected(void);

void pcm_usb_activate(UX_SLAVE_CLASS_CDC_ACM* cdc);

void pcm_usb_deactivate(void);

UINT pcm_usb_write(pcm_usb_endpoint_t endpoint, const void* buffer, ULONG length);

UX_SLAVE_CLASS_CDC_ACM* pcm_usb_cdc(void);

VOID pcm_usb_tx_thread_entry(ULONG thread_input);
VOID pcm_usb_rx_thread_entry(ULONG thread_input);

#endif //PCM_USB_H
