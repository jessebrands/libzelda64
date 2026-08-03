/*
 * options.h: command line argument parser
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

#ifndef ZELDA64_OPTIONS_H
#define ZELDA64_OPTIONS_H

#include <stdbool.h>
#include <stdio.h>

enum zelda64_mode {
    ZELDA64_MODE_NONE = 0,
    ZELDA64_MODE_INFO,
    ZELDA64_MODE_LIST,
    ZELDA64_MODE_EXTRACT,
    ZELDA64_MODE_COMPRESS,
    ZELDA64_MODE_DECOMPRESS,
};

enum zelda64_parse_status {
    ZELDA64_PARSE_OK,
    ZELDA64_PARSE_HELP,
    ZELDA64_PARSE_VERSION,
    ZELDA64_PARSE_ERROR,
};

enum zelda64_option_error {
    ZELDA64_OPTION_NONE = 0,
    ZELDA64_OPTION_UNKNOWN_LONG,
    ZELDA64_OPTION_UNKNOWN_SHORT,
    ZELDA64_OPTION_MISSING_ARGUMENT,
    ZELDA64_OPTION_BAD_VALUE,
    ZELDA64_OPTION_MODE_CONFLICT,
    ZELDA64_OPTION_MODE_MISSING,
    ZELDA64_OPTION_MODE_INVALID,
    ZELDA64_OPTION_OPERAND_MISSING,
    ZELDA64_OPTION_TOO_MANY_OPERANDS,
    ZELDA64_OPTION_STDIO,
};

struct zelda64_options {
    enum zelda64_mode mode;
    char const* rom_filename;
    char const* output_filename;
    char const* patch_filename;
    int level;
    bool verbose;
    bool overwrite;

    enum zelda64_option_error error;
    char const* error_token;
};

enum zelda64_parse_status
zelda64_parse_options(struct zelda64_options* options, int argc, char* const* argv);

char const*
zelda64_option_error_string(enum zelda64_option_error error);

void
zelda64_write_error(FILE* stream, char const* program,
                    struct zelda64_options const* options);

void
zelda64_write_usage(FILE* stream, char const* program);

#endif //ZELDA64_OPTIONS_H
