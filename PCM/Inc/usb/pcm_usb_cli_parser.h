//
// Created by Noah Husby on 9/30/26.
//

#ifndef PCM_USB_CLI_PARSER_H
#define PCM_USB_CLI_PARSER_H

#define PCM_USB_CLI_MAX_ARGS 16
#include "tx_api.h"

typedef enum
{
    PCM_USB_CLI_PARSE_OK = 0,
    PCM_USB_CLI_PARSE_TOO_MANY_ARGS,
    PCM_USB_CLI_PARSE_UNTERMINATED_QUOTE,
} PCM_USB_CLI_PARSE_RESULT;

PCM_USB_CLI_PARSE_RESULT pcm_usb_cli_parser_parse(char* line, UINT* argc, char* argv[]);

#endif //PCM_USB_CLI_PARSER_H
