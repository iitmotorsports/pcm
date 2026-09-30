#include "pcm_usb.h"
#include "pcm_usb_console.h"
//
// Created by Noah Husby on 9/30/26.
//
static pcm_usb_t usb;

static pcm_usb_packet_t* pcm_usb_tx_packet_allocate(void)
{
    pcm_usb_packet_t* packet = UX_NULL;

    tx_mutex_get(&usb.tx_packet_mutex, TX_WAIT_FOREVER);

    for (UINT i = 0; i < PCM_USB_TX_QUEUE_DEPTH; i++)
    {
        if (!usb.tx_packets[i].allocated)
        {
            usb.tx_packets[i].allocated = true;
            packet = &usb.tx_packets[i];
            break;
        }
    }

    tx_mutex_put(&usb.tx_packet_mutex);

    return packet;
}

static void pcm_usb_tx_packet_free(pcm_usb_packet_t* packet)
{
    tx_mutex_get(&usb.tx_packet_mutex, TX_WAIT_FOREVER);

    packet->allocated = false;
    packet->length = 0;

    tx_mutex_put(&usb.tx_packet_mutex);
}

void pcm_usb_init(void)
{
    memset(&usb, 0, sizeof(usb));
    usb.connected = false;
    usb.console_session_started = false;
    usb.cdc = UX_NULL;
}

UINT pcm_usb_create(TX_BYTE_POOL* byte_pool)
{
    UINT status = tx_byte_allocate(byte_pool, &usb.tx_stack, PCM_USB_THREAD_STACK_SIZE, TX_NO_WAIT);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_byte_allocate(byte_pool, &usb.rx_stack, PCM_USB_THREAD_STACK_SIZE, TX_NO_WAIT);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_byte_allocate(byte_pool, &usb.tx_queue_memory, PCM_USB_TX_QUEUE_DEPTH * sizeof(VOID*), TX_NO_WAIT);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_queue_create(&usb.tx_queue, "USB TX", PCM_USB_QUEUE_MESSAGE_WORDS, usb.tx_queue_memory,
                             PCM_USB_TX_QUEUE_DEPTH * sizeof(VOID*));

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_mutex_create(&usb.tx_packet_mutex, "USB TX Packet Mutex", TX_NO_INHERIT);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_thread_create(&usb.tx_thread, "USB TX", pcm_usb_tx_thread_entry, 0, usb.tx_stack,
                              PCM_USB_THREAD_STACK_SIZE, PCM_USB_THREAD_PRIORITY, PCM_USB_THREAD_PRIORITY,
                              TX_NO_TIME_SLICE, TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_thread_create(&usb.rx_thread, "USB RX", pcm_usb_rx_thread_entry, 0, usb.rx_stack,
                              PCM_USB_THREAD_STACK_SIZE, PCM_USB_THREAD_PRIORITY, PCM_USB_THREAD_PRIORITY,
                              TX_NO_TIME_SLICE, TX_AUTO_START);
    return status;
}

bool pcm_usb_connected(void) { return usb.connected; }

void pcm_usb_activate(UX_SLAVE_CLASS_CDC_ACM* cdc)
{
    usb.cdc = cdc;
    usb.connected = true;
}

void pcm_usb_deactivate(void)
{
    pcm_usb_console_disconnected();
    usb.console_session_started = false;
    usb.connected = false;
    usb.cdc = UX_NULL;
}

UINT pcm_usb_write(pcm_usb_endpoint_t endpoint, const void* buffer, ULONG length)
{
    if (length > PCM_USB_TX_BUFFER_SIZE)
    {
        return UX_ERROR;
    }

    pcm_usb_packet_t* packet = pcm_usb_tx_packet_allocate();
    if (packet == UX_NULL)
    {
        return UX_ERROR;
    }

    packet->endpoint = endpoint;
    packet->length = length;
    memcpy(packet->data, buffer, length);

    UINT status = tx_queue_send(&usb.tx_queue, &packet, TX_WAIT_FOREVER);

    if (status != TX_SUCCESS)
    {
        pcm_usb_tx_packet_free(packet);
    }

    return status;
}

UX_SLAVE_CLASS_CDC_ACM* pcm_usb_cdc(void) { return usb.cdc; }

VOID pcm_usb_tx_thread_entry(ULONG thread_input)
{
    UX_PARAMETER_NOT_USED(thread_input);

    while (1)
    {
        pcm_usb_packet_t* packet;

        UINT status = tx_queue_receive(&usb.tx_queue, &packet, TX_WAIT_FOREVER);

        if (status != TX_SUCCESS)
        {
            continue;
        }

        if (!usb.connected)
        {
            pcm_usb_tx_packet_free(packet);
            continue;
        }

        switch (packet->endpoint)
        {
        case PCM_USB_ENDPOINT_CONSOLE:
        {
            ULONG actual_length;
#ifndef UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE
            usb.cdc->ux_slave_class_cdc_acm_transmission_status = UX_FALSE;
#endif
            ux_device_class_cdc_acm_write(usb.cdc, packet->data, packet->length, &actual_length);

            break;
        }

        case PCM_USB_ENDPOINT_VENDOR:
        {
            /* TODO */
            break;
        }

        default:
        {
            break;
        }
        }

        pcm_usb_tx_packet_free(packet);
    }
}

VOID pcm_usb_rx_thread_entry(ULONG thread_input)
{
    UX_PARAMETER_NOT_USED(thread_input);

    while (1)
    {
        if (!usb.connected || usb.cdc == UX_NULL)
        {
            tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 10);
            continue;
        }

        UCHAR buffer[PCM_USB_MAX_PACKET_SIZE];
        ULONG actual_length = 0;
#ifndef UX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE
        usb.cdc->ux_slave_class_cdc_acm_transmission_status = UX_FALSE;
#endif
        UINT status = ux_device_class_cdc_acm_read(usb.cdc, buffer, 64, &actual_length);

        if (status != UX_SUCCESS || actual_length == 0)
        {
            continue;
        }

        if (!usb.console_session_started)
        {
            usb.console_session_started = true;
            pcm_usb_console_connected();

            /* We will discard the first packet that established the connection. */
            continue;
        }

        /* TODO: Determine destination endpoint. */
        pcm_usb_endpoint_t endpoint = PCM_USB_ENDPOINT_CONSOLE;

        switch (endpoint)
        {
        case PCM_USB_ENDPOINT_CONSOLE:
            pcm_usb_console_process_input(buffer, actual_length);
            break;

        case PCM_USB_ENDPOINT_VENDOR:
            break;

        default:
            break;
        }
    }
}