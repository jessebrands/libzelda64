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

#define ENTRIES_AT_A_TIME 1024

static enum zelda64_result
find_dma_table(FILE* f, struct zelda64_dmadata_info* info) {
    if (f == NULL || info == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    // Start from the beginning of the ROM
    size_t offset = 0;

    while (true) {
        uint8_t chunk[ZELDA64_DMA_ENTRY_SIZE * ENTRIES_AT_A_TIME];

        // Load the next entries.
        fseek(f, (long int) offset, SEEK_SET);
        size_t const entries_in = fread(chunk, ZELDA64_DMA_ENTRY_SIZE, ENTRIES_AT_A_TIME, f);
        if (entries_in == 0) {
            return ZELDA64_NO_DMADATA;
        }

        size_t const got = entries_in * ZELDA64_DMA_ENTRY_SIZE;
        size_t seek_pos = 0;

        while (seek_pos + 16 <= got) {
            if (zelda64_find_dmadata_start(chunk, got, &seek_pos) != ZELDA64_OK) {
                continue;
            }

            size_t const dma_start = offset + seek_pos;
            uint8_t entries[ZELDA64_DMA_ENTRY_SIZE * 3];

            // Load the first 3 entries into memory.
            fseek(f, (long int) dma_start, SEEK_SET);
            if (fread(entries, ZELDA64_DMA_ENTRY_SIZE, 3, f) != 3) {
                return ZELDA64_NO_DMADATA;
            }

            if (zelda64_read_dmadata_info(info, entries, ZELDA64_DMA_ENTRY_SIZE * 3) != ZELDA64_OK) {
                seek_pos += ZELDA64_DMA_ENTRY_SIZE; // This entry ain't it.
                continue;
            }

            if (info->offset != dma_start) {
                seek_pos += ZELDA64_DMA_ENTRY_SIZE; // This entry ain't it.
                continue;
            }

            return ZELDA64_OK;
        }

        offset += got;
    }

    return ZELDA64_OK;
}

int main(int argc, char** argv) {
    struct decompress_options opts;
    parse_options(&opts, argc, (char const* const*) argv);

    // Open up the ROM file.
    FILE* in_file = fopen(opts.in_filename, "rb");
    if (in_file == NULL) {
        fprintf(stderr, "decompress: fatal error: could not open '%s', does the file exist?\n", opts.in_filename);
        return EXIT_FAILURE;
    }

    fseek(in_file, 0, SEEK_END);
    size_t rom_size = (size_t) ftell(in_file);
    rewind(in_file);

    if (opts.verbose) {
        fprintf(stdout, "decompress: searching for DMADATA\n");
    }

    struct zelda64_dmadata_info dma_info;
    if (find_dma_table(in_file, &dma_info) != ZELDA64_OK) {
        fprintf(stderr,
                "decompress: fatal error: no DMADATA found in '%s', "
                "is this a valid Nintendo 64 Zelda ROM?\n",
                opts.in_filename);
        return EXIT_FAILURE;
    }

    if (opts.verbose) {
        fprintf(stdout,
                "decompress: found DMADATA at offset 0x%X (%zu entries)\n",
                dma_info.offset, dma_info.count);
    }

    // Read out the ROM info.
    rewind(in_file);
    uint8_t chunk[0x1000];
    fread(chunk, 1, sizeof chunk, in_file);

    struct zelda64_rom_info rom_info;
    enum zelda64_result result = zelda64_read_rom_info(&rom_info, chunk, sizeof chunk);
    if (result != ZELDA64_OK) {
        fprintf(stderr,
                "decompress: fatal error: failed to read ROM info: %s\n",
                zelda64_result_string(result));
        return EXIT_FAILURE;
    }

    if (opts.verbose) {
        fprintf(stdout, "decompress: ROM is %.4s version %d\n",
                rom_info.header.game_code, rom_info.header.version + 1);
        fprintf(stdout, "decompress: check code is %016lX\n", rom_info.header.check_code);
        fprintf(stdout, "decompress: protection chip is %s\n", zelda64_cic_name(rom_info.cic));
    }

    return EXIT_SUCCESS;
}
