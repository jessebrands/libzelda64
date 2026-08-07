/*
 * main.c: Program entrypoint
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of zelda64.
 *
 * zelda64 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * zelda64 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with zelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <zelda64/zelda64.h>

#include "actions.h"

#define EXIT_USAGE 2

enum zelda64_option {
    OPT_INFO = 'i',
    OPT_LIST = 'l',
    OPT_VERBOSE = 'v',
    OPT_VERSION = 256,
    OPT_HELP = 257,
};

enum zelda64_command {
    COMMAND_NONE = 0,
    COMMAND_INFO,
    COMMAND_LIST,
};

enum zelda64_parse_result {
    PARSE_OK,
    PARSE_DONE,
    PARSE_USAGE,
};

struct zelda64_options {
    bool verbose;
    enum zelda64_command command;

    char const* in_filename;
    unsigned int actions;
};

static void
show_usage(FILE* f) {
    fprintf(f, "Usage: zelda64\n");
    fprintf(f, "Manipulate Nintendo 64 Zelda ROMs.\n");
    fprintf(f, "\n");
    fprintf(f, "Options:\n");
    fprintf(f, "  -v, --verbose        write verbose messages\n");
    fprintf(f, "      --version        output version information\n");
    fprintf(f, "      --help           show this help message\n");
    fprintf(f, "\n");
}

static void
print_version(FILE* f) {
    fprintf(f, "zelda64 %s\n", zelda64_version_string());
    fprintf(f, "Copyright (C) 2026  Jesse Gerard Brands\n");
    fprintf(f, "This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(f, "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
}

static bool
set_command(struct zelda64_options* options, enum zelda64_command const command) {
    if (options->command != COMMAND_NONE) {
        return false;
    }
    options->command = command;
    return true;
}

static enum zelda64_parse_result
usage_error(char const* const message) {
    fprintf(stderr, "zelda64: error: %s\n", message);
    return PARSE_USAGE;
}

static enum zelda64_parse_result
parse_options(struct zelda64_options* options, int argc, char** argv) {
    struct option const long_options[] = {
        {"info", no_argument, NULL, OPT_INFO},
        {"list", no_argument, NULL, OPT_LIST},
        {"verbose", no_argument, NULL, OPT_VERBOSE},
        {"help", no_argument, NULL, OPT_HELP},
        {"version", no_argument, NULL, OPT_VERSION},
        {0, 0, 0, 0},
    };

    int c;

    opterr = 0;
    while ((c = getopt_long(argc, argv, ":vi", long_options, NULL)) != -1) {
        switch (c) {
            case OPT_VERBOSE:
                options->verbose = true;
                break;

            case OPT_HELP:
                show_usage(stdout);
                return PARSE_DONE;

            case OPT_VERSION:
                print_version(stdout);
                return PARSE_DONE;

            case OPT_INFO:
                if (!set_command(options, COMMAND_INFO)) {
                    return usage_error("only one command may be given");
                }
                break;

            case OPT_LIST:
                if (!set_command(options, COMMAND_LIST)) {
                    return usage_error("only one command may be given");
                }
                break;


            case '?':
                if (optopt != 0) {
                    fprintf(stderr, "zelda64: error: unrecognized option '-%c'\n", optopt);
                } else {
                    fprintf(stderr, "zelda64: error: unrecognized option '%s'\n", argv[optind - 1]);
                }
                return PARSE_USAGE;

            default:
                abort();
        }
    }

    int const arguments = argc - optind;

    if (options->command != COMMAND_NONE) {
        if (arguments < 1) {
            return usage_error("no input file given");
        }
        if (arguments > 1) {
            return usage_error("too many arguments");
        }
        options->in_filename = argv[optind];
        return PARSE_OK;
    }

    return usage_error("no command or stage(s) given");
}

int main(int argc, char** argv) {
    struct zelda64_options options = {0};
    enum zelda64_parse_result const result = parse_options(&options, argc, argv);
    switch (result) {
        case PARSE_DONE:
            return EXIT_SUCCESS;

        case PARSE_USAGE:
            fprintf(stderr, "See 'zelda64 --help' for usage information.\n\n");
            return EXIT_USAGE;

        case PARSE_OK:
            break;
    }

    // Open file and get the file size.
    FILE* in_file = fopen(options.in_filename, "rb");
    fseek(in_file, 0, SEEK_END);
    long const in_size = ftell(in_file);
    rewind(in_file);

    uint8_t* in_rom = malloc((size_t) in_size);
    if (in_rom == NULL) {
        return EXIT_FAILURE;
    }

    fread(in_rom, 1, (size_t) in_size, in_file);

    action_rom_info(in_rom, (size_t) in_size);

    free(in_rom);
    return EXIT_SUCCESS;
}
