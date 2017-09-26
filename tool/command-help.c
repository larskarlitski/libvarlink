#include "command.h"
#include "interface.h"
#include "object.h"
#include "terminal-colors.h"
#include "util.h"

#include <errno.h>
#include <getopt.h>
#include <string.h>

static long help_interface(Cli *cli,
                           VarlinkConnection *connection,
                           const char *name) {
        _cleanup_(varlink_object_unrefp) VarlinkObject *parameters = NULL;
        _cleanup_(varlink_object_unrefp) VarlinkObject *out = NULL;
        _cleanup_(freep) char *error = NULL;
        _cleanup_(varlink_interface_freep) VarlinkInterface *interface = NULL;
        const char *description = NULL;
        _cleanup_(freep) char *string = NULL;
        long r;

        varlink_object_new(&parameters);
        varlink_object_set_string(parameters, "interface", name);

        r = cli_call(cli,
                     connection,
                     "org.varlink.service.GetInterfaceDescription",
                     parameters,
                     0,
                     &error,
                     &out);
        if (r < 0)
                return r;

        if (error) {
                printf("Error: %s\n", error);
                return 0;
        }

        if (varlink_object_get_string(out, "description", &description) < 0)
                return -CLI_ERROR_CALL_FAILED;

        r = varlink_interface_new(&interface, description, NULL);
        if (r < 0)
                return -CLI_ERROR_PANIC;

        r  = varlink_interface_write_description(interface,
                                                 &string,
                                                 0,
                                                 terminal_color(TERMINAL_BLUE), terminal_color(TERMINAL_NORMAL),
                                                 terminal_color(TERMINAL_MAGENTA), terminal_color(TERMINAL_NORMAL),
                                                 terminal_color(TERMINAL_GREEN), terminal_color(TERMINAL_NORMAL),
                                                 terminal_color(TERMINAL_CYAN), terminal_color(TERMINAL_NORMAL));
        if (r < 0)
                return r;

        printf("%s\n", string);

        return 0;
}

static long help_run(Cli *cli, int argc, char **argv) {
        static const struct option options[] = {
                { "help",    no_argument,       NULL, 'h' },
                {}
        };
        _cleanup_(varlink_connection_freep) VarlinkConnection *connection = NULL;
        bool ssh;
        _cleanup_(freep) char *address = NULL;
        unsigned int port;
        _cleanup_(freep) char *interface = NULL;
        int c;
        long r;

        while ((c = getopt_long(argc, argv, "a:h", options, NULL)) >= 0) {
                switch (c) {
                        case 'h':
                                printf("Usage: %s help [ADDRESS/]INTERFACE\n", program_invocation_short_name);
                                printf("\n");
                                printf("Prints information about INTERFACE.\n");
                                printf("\n");
                                printf("  -h, --help             display this help text and exit\n");
                                return 0;

                        default:
                                fprintf(stderr, "Try '%s --help' for more information\n",
                                        program_invocation_short_name);
                                return -CLI_ERROR_INVALID_ARGUMENT;
                }
        }

        if (!argv[optind]) {
                fprintf(stderr, "Usage: %s help [ADDRESS/]INTERFACE\n", program_invocation_short_name);
                return -CLI_ERROR_MISSING_ARGUMENT;
        }

        r = cli_parse_url(argv[optind],
                          false,
                          &ssh,
                          &address,
                          &port,
                          &interface);
        if (r < 0)
                return r;

        r = cli_connect(cli,
                        &connection,
                        ssh,
                        address,
                        port,
                        interface);
        if (r < 0)
                return r;

        r = help_interface(cli, connection, interface);
        if (r < 0)
                return r;

        return 0;
}

static long help_complete(Cli *cli, int argc, char **argv, const char *current) {
        return cli_complete_interfaces(cli, current, false);
}

const CliCommand command_help = {
        .name = "help",
        .info = "Print interface description or service information",
        .run = help_run,
        .complete = help_complete
};
