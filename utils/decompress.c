/*
 * decompress.c: Nintendo 64 Zelda ROM decompression utility
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * Decompress is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Decompress is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Decompress. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zelda64/zelda64.h>

#define EXIT_USAGE 2

struct decompress_options {
    char const* in_filename;
    char const* out_filename;
    bool verbose;
};

static void
print_version(void) {
    fprintf(stdout,
            "decompress (zelda64-utils) %s\n"
            "Copyright (C) 2026  Jesse Gerard Brands\n"
            "This is free software; see the source for copying conditions.  There is NO\n"
            "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n",
            zelda64_version_string());
}

static void
print_usage(FILE* stream, char const* program) {
    fprintf(stream, "Usage: %s rom outfile\n", program);
    fprintf(stream, "Decompress a Nintendo 64 Zelda ROM.\n");
    fprintf(stream, "\n");
    fprintf(stream, "Options:\n");
    fprintf(stream, "  -v, --verbose          write verbose messages to stdout\n");
    fprintf(stream, "      --version          output version information and exit\n");
    fprintf(stream, "      --help             show this help message\n");
    fprintf(stream, "\n");
}

static void
parse_options(struct decompress_options* opts, int const argc, char const* const* argv) {
    *opts = (struct decompress_options){0};

    for (int i = 1; i < argc; ++i) {
        char const* arg = argv[i];

        if (arg[0] != '-') {
            if (opts->in_filename == NULL) {
                opts->in_filename = arg;
                continue;
            }
            if (opts->out_filename == NULL) {
                opts->out_filename = arg;
                continue;
            }
            fprintf(stderr, "decompress: error: too many arguments\n\n");
            exit(EXIT_FAILURE);
        }

        if (arg[1] != '\0' && arg[1] == '-') {
            char const* opt = &arg[2];

            if (strcmp(opt, "version") == 0) {
                print_version();
                exit(EXIT_SUCCESS);
            }
            if (strcmp(opt, "help") == 0) {
                print_usage(stdout, argv[0]);
                exit(EXIT_SUCCESS);
            }
            if (strcmp(opt, "verbose") == 0) {
                opts->verbose = true;
                continue;
            }

            continue;
        }

        if (arg[1] == '\0') {
            fprintf(stderr, "decompress: error: stdio is not supported\n\n");
            exit(EXIT_FAILURE);
        }

        switch (arg[1]) {
            case 'v':
                opts->verbose = true;
                continue;

            default:
                print_usage(stderr, argv[0]);
                exit(EXIT_USAGE);
        }
    }

    if (opts->in_filename == NULL) {
        fprintf(stderr, "decompress: fatal error: no input ROM\n\n");
        exit(EXIT_FAILURE);
    }
    if (opts->out_filename == NULL) {
        fprintf(stderr, "decompress: fatal error: no output file specified\n\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {
    struct decompress_options opts;
    parse_options(&opts, argc, (char const* const*) argv);

    return EXIT_SUCCESS;
}
