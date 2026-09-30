//
// Created by Noah Husby on 9/30/26.
//

#ifndef PCM_USB_CONSOLE_H
#define PCM_USB_CONSOLE_H

#define PCM_USB_CONSOLE_PRINTF_BUFFER_SIZE 256
#define PCM_USB_CONSOLE_WRITE_BUFFER_SIZE 4096

#define PCM_USB_CONSOLE_HISTORY_SIZE 8

#include "ux_api.h"
#include <stdarg.h>

typedef struct
{
    char entries[PCM_USB_CONSOLE_HISTORY_SIZE][PCM_USB_CONSOLE_PRINTF_BUFFER_SIZE];

    UINT count;
    UINT head;
    INT index;
} pcm_usb_console_history_t;

void pcm_usb_console_init(void);

UINT pcm_usb_console_write(const void* buffer, ULONG length);
UINT pcm_usb_console_write_string(const char* string);
UINT pcm_usb_console_printf(const char* format, ...);

UINT pcm_usb_console_read(void* buffer, ULONG buffer_length, ULONG* actual_length);

void pcm_usb_console_process_input(const UCHAR* data, ULONG length);

void pcm_usb_console_connected(void);
void pcm_usb_console_disconnected(void);

void pcm_usb_console_begin_write(void);
UINT pcm_usb_console_end_write(void);

void pcm_usb_console_clear(void);


#endif //PCM_USB_CONSOLE_H
