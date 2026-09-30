//
// Created by Noah Husby on 9/30/26.
//
#include "pcm_usb_cli_parser.h"

#include <stdbool.h>

PCM_USB_CLI_PARSE_RESULT pcm_usb_cli_parser_parse(char* line, UINT* argc, char* argv[])
{
    char* read = line;
    char* write = line;

    *argc = 0;

    while (*read != '\0')
    {
        while (*read == ' ' || *read == '\t' || *read == '\r' || *read == '\n')
        {
            read++;
        }

        if (*read == '\0')
        {
            break;
        }

        if (*argc >= PCM_USB_CLI_MAX_ARGS)
        {
            return PCM_USB_CLI_PARSE_TOO_MANY_ARGS;
        }

        argv[*argc] = write;
        (*argc)++;

        bool in_quotes = false;
        bool closed_quote = false;

        if (*read == '"')
        {
            in_quotes = true;
            read++;
        }

        while (*read != '\0')
        {
            if (in_quotes)
            {
                if (*read == '"')
                {
                    closed_quote = true;
                    read++;
                    break;
                }
            }
            else
            {
                if (*read == ' ' || *read == '\t' || *read == '\r' || *read == '\n')
                {
                    break;
                }
            }

            if (*read == '\\')
            {
                read++;

                switch (*read)
                {
                case '\0':
                    *write++ = '\\';
                    break;

                case 'n':
                    *write++ = '\n';
                    read++;
                    break;

                case 'r':
                    *write++ = '\r';
                    read++;
                    break;

                case 't':
                    *write++ = '\t';
                    read++;
                    break;

                case '"':
                    *write++ = '"';
                    read++;
                    break;

                case '\\':
                    *write++ = '\\';
                    read++;
                    break;

                default:
                    *write++ = *read++;
                    break;
                }
            }
            else
            {
                *write++ = *read++;
            }
        }

        if (in_quotes && !closed_quote)
        {
            return PCM_USB_CLI_PARSE_UNTERMINATED_QUOTE;
        }

        while (*read == ' ' || *read == '\t' || *read == '\r' || *read == '\n')
        {
            read++;
        }

        *write++ = '\0';
    }

    return PCM_USB_CLI_PARSE_OK;
}
