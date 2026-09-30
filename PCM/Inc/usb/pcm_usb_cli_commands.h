//
// Created by Noah Husby on 9/30/26.
//

#ifndef PCM_USB_CLI_COMMANDS_H
#define PCM_USB_CLI_COMMANDS_H
#include "tx_api.h"
#include "pcm_usb_cli.h"

const PCM_USB_CLI_COMMAND* pcm_usb_cli_commands_get_root(void);
UINT pcm_usb_cli_commands_get_root_count(void);

#endif //PCM_USB_CLI_COMMANDS_H
