//
// Created by Noah Husby on 9/30/26.
//
#define ARRAY_COUNT(x) ((UINT)(sizeof(x) / sizeof((x)[0])))
#define PCM_USB_CLI_GPIB_BUFFER_SIZE 256
#include <stdint.h>

#include "pcm_usb_cli.h"
#include "pcm_usb_console.h"
#include "main.h"
static UINT cmd_ping(UINT argc, char* argv[]);
static UINT cmd_system_reboot(UINT argc, char* argv[]);
static UINT cmd_clear(UINT argc, char* argv[]);

static const PCM_USB_CLI_COMMAND system_commands[] = {
    {
        .name = "reboot",
        .description = "Reboot the controller",
        .usage = "system reboot",
        .handler = cmd_system_reboot,
    },
};

static const PCM_USB_CLI_COMMAND root_commands[] = {
    {
        .name = "help",
        .description = "Show command help",
        .usage = "help [command] [subcommand]",
        .handler = TX_NULL,
    },
    {
        .name = "clear",
        .description = "Clear the console",
        .usage = "clear",
        .handler = cmd_clear,
    },
    {
        .name = "ping",
        .description = "Display a test message",
        .usage = "ping",
        .handler = cmd_ping,
    },
    {
        .name = "system",
        .description = "System commands",
        .usage = "system <command>",
        .children = system_commands,
        .child_count = ARRAY_COUNT(system_commands),
    },
};

const PCM_USB_CLI_COMMAND* pcm_usb_cli_commands_get_root(void) { return root_commands; }

UINT pcm_usb_cli_commands_get_root_count(void) { return ARRAY_COUNT(root_commands); }

static UINT cmd_ping(UINT argc, char* argv[])
{

    (void)argc;
    (void)argv;

    pcm_usb_console_write_string("Pong!\r\n");
    return TX_SUCCESS;
}

static UINT cmd_system_reboot(UINT argc, char* argv[])
{
    (void)argc;
    (void)argv;

    HAL_NVIC_SystemReset();
    return TX_SUCCESS;
}

static UINT cmd_clear(UINT argc, char* argv[])
{
    (void)argc;
    (void)argv;

    pcm_usb_console_clear();

    return TX_SUCCESS;
}