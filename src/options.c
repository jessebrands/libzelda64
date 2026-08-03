/*
 * options.c: command line argument parser implementation
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of zelda64.
 *
 * zelda64 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * zelda64 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * zelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zelda64/zelda64.h>

#include "options.h"

#define MODE_BIT(m) (1u << (unsigned) (m))

#define WRITING_MODES (MODE_BIT(ZELDA64_MODE_EXTRACT)   \
                     | MODE_BIT(ZELDA64_MODE_COMPRESS)  \
                     | MODE_BIT(ZELDA64_MODE_DECOMPRESS))

enum {
    OPT_HELP = 'h',
    OPT_INFORMATION = 'i',
    OPT_DMA_TABLE = 't',
    OPT_VERBOSE = 'v',
    OPT_VERSION = 256,
};

struct zelda64_option {
    char short_name;
    char const* long_name;
    bool has_argument;
    int id;
    unsigned valid_modes;
    char const* argument_name;
    char const* description;
};

// @formatter:off
static struct zelda64_option const options_table[] = {
    {'i', "info",      false, OPT_INFORMATION, 0, NULL, "show ROM information"},
    {'t', "dma-table", false, OPT_DMA_TABLE,   0, NULL, "show DMA table"},
    {'v', "verbose",   false, OPT_VERBOSE,     0, NULL, "display verbose messages"},
    {0,   "version",   false, OPT_VERSION,     0, NULL, "display version information"},
    {'h', "help",      false, OPT_HELP,        0, NULL, "display this message"},
};
// @formatter:on

#define OPTIONS_COUNT (sizeof options_table / sizeof options_table[0])

static struct zelda64_option const*
find_short_option(char const name) {
    for (size_t i = 0; i < OPTIONS_COUNT; ++i) {
        if (options_table[i].short_name == 0) {
            continue;
        }
        if (options_table[i].short_name == name) {
            return &options_table[i];
        }
    }
    return NULL;
}

static struct zelda64_option const*
find_long_option(char const* name) {
    for (size_t i = 0; i < OPTIONS_COUNT; ++i) {
        if (options_table[i].long_name == NULL) {
            continue;
        }
        if (strcmp(options_table[i].long_name, name) == 0) {
            return &options_table[i];
        }
    }
    return NULL;
}

static enum zelda64_parse_status
parser_error(struct zelda64_options* options, enum zelda64_option_error const error, char const* arg) {
    options->error = error;
    options->error_token = arg;
    return ZELDA64_PARSE_ERROR;
}

static size_t
expected_operands(enum zelda64_mode const mode) {
    switch (mode) {
        case ZELDA64_MODE_INFO:
        case ZELDA64_MODE_LIST:
            return 1;

        case ZELDA64_MODE_EXTRACT:
        case ZELDA64_MODE_COMPRESS:
        case ZELDA64_MODE_DECOMPRESS:
            return 2;

        case ZELDA64_MODE_NONE:
            return 0;
    }
    // Prefer this over default, it triggers missing case warnings.
    return 0;
}

static enum zelda64_mode
mode_for(int const id) {
    switch (id) {
        case OPT_INFORMATION:
            return ZELDA64_MODE_INFO;

        case OPT_DMA_TABLE:
            return ZELDA64_MODE_LIST;
    }
    // Prefer this over default, it triggers missing case warnings.
    return ZELDA64_MODE_NONE;
}

static bool
set_mode(struct zelda64_options* options, enum zelda64_mode const mode) {
    if (mode == ZELDA64_MODE_NONE) {
        return false;
    }
    if (options->mode != ZELDA64_MODE_NONE && options->mode != mode) {
        return false;
    }
    options->mode = mode;
    return true;
}

static enum zelda64_parse_status
apply_option(struct zelda64_options* options, struct zelda64_option const* opt, char const* value, char const* arg) {
    (void) value;

    switch (opt->id) {
        case OPT_INFORMATION:
        case OPT_DMA_TABLE:
            if (!set_mode(options, mode_for(opt->id))) {
                return parser_error(options, ZELDA64_OPTION_MODE_CONFLICT, arg);
            }
            break;

        case OPT_VERBOSE:
            options->verbose = true;
            break;

        case OPT_HELP: return ZELDA64_PARSE_HELP;
        case OPT_VERSION: return ZELDA64_PARSE_VERSION;

        default:
            break;
    }
    return ZELDA64_PARSE_OK;
}

enum zelda64_parse_status
zelda64_parse_options(struct zelda64_options* options, int const argc, char* const* argv) {
    *options = (struct zelda64_options){
        .level = ZELDA64_DEFAULT_COMPRESSION
    };

    bool end = false;
    char const* operands[] = {NULL, NULL};
    size_t operand_count = 0;

    // Bitmask that stores which options were passed.
    unsigned seen = 0;

    for (int i = 1; i < argc; ++i) {
        char const* arg = argv[i];

        if (end || arg[0] != '-' || arg[1] == '\0') {
            // Can't take `stdio`, we need seekable buffers.
            if (!end && arg[0] == '-') {
                return parser_error(options, ZELDA64_OPTION_STDIO, arg);
            }

            if (operand_count >= (sizeof operands / sizeof *operands)) {
                return parser_error(options, ZELDA64_OPTION_TOO_MANY_OPERANDS, arg);
            }

            operands[operand_count++] = arg;
            continue;
        }

        if (arg[1] == '-') {
            // We stop parsing at `--`.
            if (arg[2] == '\0') {
                end = true;
                continue;
            }

            char const* name = &arg[2];
            struct zelda64_option const* opt = find_long_option(name);
            if (opt == NULL) {
                return parser_error(options, ZELDA64_OPTION_UNKNOWN_LONG, arg);
            }

            char const* value = NULL;
            if (opt->has_argument) {
                if (++i >= argc) {
                    return parser_error(options, ZELDA64_OPTION_MISSING_ARGUMENT, arg);
                }
                value = argv[i];
            }

            seen |= 1u << (opt - options_table);

            enum zelda64_parse_status const status = apply_option(options, opt, value, arg);
            if (status != ZELDA64_PARSE_OK) {
                return status;
            }
        } else {
            for (char const* p = &arg[1]; *p != '\0'; ++p) {
                struct zelda64_option const* opt = find_short_option(*p);
                if (opt == NULL) {
                    return parser_error(options, ZELDA64_OPTION_UNKNOWN_SHORT, p);
                }

                char const* value = NULL;
                if (opt->has_argument) {
                    if (++i >= argc) {
                        return parser_error(options, ZELDA64_OPTION_MISSING_ARGUMENT, arg);
                    }
                    value = argv[i];
                }

                seen |= 1u << (opt - options_table);

                enum zelda64_parse_status const status = apply_option(options, opt, value, arg);
                if (status != ZELDA64_PARSE_OK) {
                    return status;
                }
            }
        }
    }

    if (options->mode == ZELDA64_MODE_NONE) {
        return parser_error(options, ZELDA64_OPTION_MODE_MISSING, NULL);
    }

    // Make sure the options we were passed are valid for the selected mode.
    for (size_t i = 0; i < OPTIONS_COUNT; ++i) {
        if ((seen & 1u << i) == 0 || options_table[i].valid_modes == 0) {
            continue;
        }
        if ((options_table[i].valid_modes & MODE_BIT(options->mode)) == 0) {
            return parser_error(options, ZELDA64_OPTION_MODE_INVALID, options_table[i].long_name);
        }
    }

    options->rom_filename = operands[0];
    options->output_filename = operands[1];

    size_t const expected = expected_operands(options->mode);
    if (operand_count < expected) {
        return parser_error(options, ZELDA64_OPTION_OPERAND_MISSING, NULL);
    }
    if (operand_count > expected) {
        return parser_error(options, ZELDA64_OPTION_TOO_MANY_OPERANDS, operands[expected]);
    }

    return ZELDA64_PARSE_OK;
}

char const*
zelda64_option_error_string(enum zelda64_option_error const error) {
    switch (error) {
        case ZELDA64_OPTION_NONE: return "no error";
        case ZELDA64_OPTION_UNKNOWN_LONG:
        case ZELDA64_OPTION_UNKNOWN_SHORT: return "unrecognized option";
        case ZELDA64_OPTION_MISSING_ARGUMENT: return "option requires an argument";
        case ZELDA64_OPTION_BAD_VALUE: return "invalid argument";
        case ZELDA64_OPTION_MODE_CONFLICT: return "only one mode may be given";
        case ZELDA64_OPTION_MODE_MISSING: return "no mode given";
        case ZELDA64_OPTION_MODE_INVALID: return "option is not valid for this mode";
        case ZELDA64_OPTION_OPERAND_MISSING: return "missing operand";
        case ZELDA64_OPTION_TOO_MANY_OPERANDS: return "unexpected operand";
        case ZELDA64_OPTION_STDIO: return "standard input/output is not supported";
    }
    return "unknown error";
}

void zelda64_write_error(FILE* stream, char const* program, struct zelda64_options const* options) {
    fprintf(stream, "%s: ", program);

    char const* what = zelda64_option_error_string(options->error);

    switch (options->error) {
        case ZELDA64_OPTION_UNKNOWN_LONG:
            fprintf(stream, "%s '%s'", what, options->error_token);
            break;

        case ZELDA64_OPTION_UNKNOWN_SHORT:
            fprintf(stream, "%s -- '%c'", what, options->error_token[0]);
            break;

        case ZELDA64_OPTION_MISSING_ARGUMENT:
            fprintf(stream, "argument '%s' is missing", options->error_token);
            break;

        case ZELDA64_OPTION_BAD_VALUE:
            fprintf(stream, "invalid value '%s'", options->error_token);
            break;

        case ZELDA64_OPTION_OPERAND_MISSING:
            if (options->rom_filename == NULL) {
                fprintf(stream, "missing input rom");
            } else {
                fprintf(stream, "missing output path");
            }
            break;

        case ZELDA64_OPTION_TOO_MANY_OPERANDS:
            fprintf(stream, "unexpected operand: %s", options->error_token);
            break;

        default:
            fprintf(stream, "%s", what);
            break;
    }

    fprintf(stream, "\n");
    fprintf(stream, "Try '%s --help' for more information.\n", program);
}

static void
write_option(FILE* stream, struct zelda64_option const* opt) {
    char const* const sep = opt->argument_name != NULL ? " " : "";
    char const* const name = opt->argument_name != NULL ? opt->argument_name : "";
    char left[40] = "";

    if (opt->short_name != 0 && opt->long_name != 0) {
        snprintf(left, sizeof left, "-%c, --%s%s%s", opt->short_name, opt->long_name, sep, name);
    } else if (opt->long_name != 0) {
        snprintf(left, sizeof left, "    --%s%s%s", opt->long_name, sep, name);
    } else if (opt->short_name != 0) {
        snprintf(left, sizeof left, "-%c", opt->short_name);
    }

    fprintf(stream, "  %-20s %s\n", left, opt->description);
}

void zelda64_write_usage(FILE* stream, char const* program) {
    fprintf(stream, "usage: %s mode [options...] rom [out]\n\n", program);

    fprintf(stream, "Operation mode:\n");
    for (size_t i = 0; i < OPTIONS_COUNT; ++i) {
        if (mode_for(options_table[i].id) != ZELDA64_MODE_NONE) {
            write_option(stream, &options_table[i]);
        }
    }

    fprintf(stream, "\nOptions:\n");
    for (size_t i = 0; i < OPTIONS_COUNT; ++i) {
        if (mode_for(options_table[i].id) == ZELDA64_MODE_NONE) {
            write_option(stream, &options_table[i]);
        }
    }
}
