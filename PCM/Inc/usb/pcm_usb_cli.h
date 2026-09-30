//
// Created by Noah Husby on 9/30/26.
//

#ifndef PCM_USB_CLI_H
#define PCM_USB_CLI_H

#include "tx_api.h"

#define PCM_USB_CLI_MAX_ARGS 16
#define PCM_USB_CLI_MAX_LINE_LENGTH 256

typedef UINT (*PCM_USB_CLI_HANDLER)(UINT argc, char* argv[]);

typedef struct PCM_USB_CLI_COMMAND
{
    const char* name;
    const char* description;
    const char* usage;

    PCM_USB_CLI_HANDLER handler;

    const struct PCM_USB_CLI_COMMAND* children;
    UINT child_count;
} PCM_USB_CLI_COMMAND;

void pcm_usb_cli_start(void);
void pcm_usb_cli_process_line(char* line);
void pcm_usb_cli_print_prompt(void);


#endif //PCM_USB_CLI_H
