//
// Created by Noah Husby on 9/30/26.
//

#include "pcm_usb_cli_commands.h"
#include "pcm_usb_cli_parser.h"
#include "pcm_usb_console.h"

static const PCM_USB_CLI_COMMAND* usb_cli_find_command(const PCM_USB_CLI_COMMAND* commands, UINT command_count,
                                                        const char* name);

static void usb_cli_execute(UINT argc, char* argv[]);
static void usb_cli_execute_help(UINT argc, char* argv[]);
static void usb_cli_print_command_list(const PCM_USB_CLI_COMMAND* commands, UINT command_count);
static void usb_cli_print_command_help(const PCM_USB_CLI_COMMAND* command);
static void usb_cli_print_unknown_command(const char* name);

static void usb_cli_print_banner(void)
{
    pcm_usb_console_printf("\r\n"
                            "=====================================================\r\n"
                            " Propulsion Control Module (PCM)\r\n"
                            " Designed by IIT Motorsports | Firmware v%s\r\n"
                            "\r\n"
                            " Type 'help' for available commands.\r\n"
                            "=====================================================\r\n"
                            "\r\n",
                            "0.1.0");
}

void pcm_usb_cli_print_prompt(void) { pcm_usb_console_write_string("> "); }

void pcm_usb_cli_start(void)
{
    usb_cli_print_banner();
    pcm_usb_cli_print_prompt();
}

void pcm_usb_cli_process_line(char* line)
{
    UINT argc = 0;
    char* argv[PCM_USB_CLI_MAX_ARGS];

    PCM_USB_CLI_PARSE_RESULT parse_result = pcm_usb_cli_parser_parse(line, &argc, argv);

    switch (parse_result)
    {
    case PCM_USB_CLI_PARSE_OK:
        break;

    case PCM_USB_CLI_PARSE_TOO_MANY_ARGS:
        pcm_usb_console_write_string("Syntax error: too many arguments\r\n");
        pcm_usb_cli_print_prompt();
        return;

    case PCM_USB_CLI_PARSE_UNTERMINATED_QUOTE:
        pcm_usb_console_write_string("Syntax error: unterminated quote\r\n");
        pcm_usb_cli_print_prompt();
        return;

    default:
        pcm_usb_console_write_string("Syntax error\r\n");
        pcm_usb_cli_print_prompt();
        return;
    }

    if (argc > 0)
    {
        usb_cli_execute(argc, argv);
    }

    pcm_usb_cli_print_prompt();
}

static void usb_cli_execute(UINT argc, char* argv[])
{
    if (strcmp(argv[0], "help") == 0)
    {
        usb_cli_execute_help(argc, argv);
        return;
    }

    const PCM_USB_CLI_COMMAND* commands = pcm_usb_cli_commands_get_root();

    UINT command_count = pcm_usb_cli_commands_get_root_count();

    const PCM_USB_CLI_COMMAND* command = TX_NULL;

    UINT arg_index = 0;

    while (arg_index < argc)
    {
        command = usb_cli_find_command(commands, command_count, argv[arg_index]);

        if (command == TX_NULL)
        {
            usb_cli_print_unknown_command(argv[arg_index]);
            return;
        }

        arg_index++;

        if (command->children != TX_NULL && command->child_count > 0 && arg_index < argc)
        {
            commands = command->children;
            command_count = command->child_count;
            continue;
        }

        break;
    }

    if (command == TX_NULL)
    {
        return;
    }

    if (command->handler != TX_NULL)
    {
        command->handler(argc - arg_index, &argv[arg_index]);
        return;
    }

    if (command->children != TX_NULL && command->child_count > 0)
    {
        usb_cli_print_command_help(command);

        pcm_usb_console_write_string("\r\nSubcommands:\r\n");

        usb_cli_print_command_list(command->children, command->child_count);

        return;
    }

    pcm_usb_console_write_string("Command has no handler\r\n");
}

static void usb_cli_execute_help(UINT argc, char* argv[])
{
    const PCM_USB_CLI_COMMAND* commands = pcm_usb_cli_commands_get_root();

    UINT command_count = pcm_usb_cli_commands_get_root_count();

    const PCM_USB_CLI_COMMAND* command = TX_NULL;

    if (argc == 1)
    {
        pcm_usb_console_write_string("Available commands:\r\n");

        usb_cli_print_command_list(commands, command_count);

        return;
    }

    for (UINT i = 1; i < argc; i++)
    {
        command = usb_cli_find_command(commands, command_count, argv[i]);

        if (command == TX_NULL)
        {
            usb_cli_print_unknown_command(argv[i]);
            return;
        }

        commands = command->children;
        command_count = command->child_count;
    }

    usb_cli_print_command_help(command);

    if (command->children != TX_NULL && command->child_count > 0)
    {
        pcm_usb_console_write_string("\r\nSubcommands:\r\n");

        usb_cli_print_command_list(command->children, command->child_count);
    }
}

static const PCM_USB_CLI_COMMAND* usb_cli_find_command(const PCM_USB_CLI_COMMAND* commands, UINT command_count,
                                                        const char* name)
{
    if (commands == TX_NULL || name == TX_NULL)
    {
        return TX_NULL;
    }

    for (UINT i = 0; i < command_count; i++)
    {
        if (commands[i].name != TX_NULL && strcmp(commands[i].name, name) == 0)
        {
            return &commands[i];
        }
    }

    return TX_NULL;
}

static void usb_cli_print_command_list(const PCM_USB_CLI_COMMAND* commands, UINT command_count)
{
    pcm_usb_console_begin_write();
    for (UINT i = 0; i < command_count; i++)
    {
        pcm_usb_console_printf("  %-12s", commands[i].name);

        if (commands[i].description != TX_NULL)
        {
            pcm_usb_console_printf("%s", commands[i].description);
        }

        pcm_usb_console_write_string("\r\n");
    }
    pcm_usb_console_end_write();
}

static void usb_cli_print_command_help(const PCM_USB_CLI_COMMAND* command)
{
    if (command == TX_NULL)
    {
        return;
    }

    pcm_usb_console_printf("%s\r\n", command->name);

    if (command->description != TX_NULL)
    {
        pcm_usb_console_printf("%s\r\n", command->description);
    }

    if (command->usage != TX_NULL)
    {
        pcm_usb_console_write_string("\r\nUsage:\r\n");

        pcm_usb_console_printf("  %s\r\n", command->usage);
    }
}

static void usb_cli_print_unknown_command(const char* name)
{
    pcm_usb_console_printf("Unknown command: %s\r\n", name);

    pcm_usb_console_write_string("Type \"help\" for available commands\r\n");
}