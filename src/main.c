/*
 * main.c: application entrypoint
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

#include "zelda64_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <zelda64/zelda64.h>

#include "commands.h"
#include "options.h"

#define EXIT_USAGE 2

static void
write_version(FILE* stream) {
    fprintf(stream,
            "%s %s\n"
            "Copyright (C) 2026  Jesse Gerard Brands\n",
            ZELDA64_PROGRAM_NAME,
            ZELDA64_VERSION_STRING);

    // Print the GPL disclaimer too
    fprintf(stream,
            "This is free software; see the source for copying conditions.  There is NO\n"
            "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

int main(int argc, char** argv) {
    struct zelda64_options options = {0};
    enum zelda64_parse_status const status = zelda64_parse_options(&options, argc, argv);

    switch (status) {
        case ZELDA64_PARSE_HELP:
            zelda64_write_usage(stdout, ZELDA64_PROGRAM_NAME);
            return EXIT_SUCCESS;

        case ZELDA64_PARSE_VERSION:
            write_version(stdout);
            return EXIT_SUCCESS;

        case ZELDA64_PARSE_ERROR:
            zelda64_write_error(stderr, ZELDA64_PROGRAM_NAME, &options);
            return EXIT_USAGE;

        case ZELDA64_PARSE_OK:
            break;
    }

    enum zelda64_result result = ZELDA64_OK;

    switch (options.mode) {
        case ZELDA64_MODE_INFO:
            result = zelda64_run_info(&options);
            break;

        case ZELDA64_MODE_LIST:
            result = zelda64_list_dma_table(&options);
            break;

        case ZELDA64_MODE_NONE:
        case ZELDA64_MODE_EXTRACT:
        case ZELDA64_MODE_COMPRESS:
        case ZELDA64_MODE_DECOMPRESS:
            break;
    }

    return result == ZELDA64_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
