/*
 * decompress.c: Nintendo 64 Zelda ROM Decompressor
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zelda64/zelda64.h>

#include "rom.h"

static int
print_error(enum zelda64_result const result, char const* what) {
    fprintf(stderr, "decompress: error: %s: ", what);
    if (result == ZELDA64_IO_ERROR) {
        fprintf(stderr, "%s\n", strerror(errno));
    } else {
        fprintf(stderr, "%s\n", zelda64_result_string(result));
    }
    return EXIT_FAILURE;
}


int main(int argc, char** argv) {
    int exit_code = EXIT_SUCCESS;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s rom outfile\n", argv[0]);
        return EXIT_FAILURE;
    }

    char const* in_filename = argv[1];
    char const* out_filename = argv[2];

    // Open the source ROM.
    struct zelda64_rom in_rom;
    enum zelda64_result result = zelda64_open_rom(in_filename, &in_rom);
    if (result != ZELDA64_OK) {
        return print_error(result, "could not open input ROM");
    }

    // Create the destination ROM.
    struct zelda64_rom out_rom;
    result = zelda64_create_rom(out_filename, DECOMPRESSED_SIZE, &in_rom.dma_info, &out_rom);
    if (result != ZELDA64_OK) {
        exit_code = print_error(result, "could not open output ROM");
        goto cleanup_in_rom;
    }

    for (size_t i = 0; i < in_rom.dma_info.count; ++i) {
        struct zelda64_dma_entry const in_entry = in_rom.dma_table[i];
        enum zelda64_dma_kind const kind = zelda64_dma_entry_kind(&in_entry);

        switch (kind) {
            case ZELDA64_DMA_EMPTY:
                fprintf(stdout, "zelda64: skipping empty file %04zX\n", i);
                out_rom.dma_table[i] = in_entry;
                break;

            case ZELDA64_DMA_DELETED:
                fprintf(stdout, "zelda64: skipping deleted file %04zX\n", i);
                out_rom.dma_table[i] = in_entry;
                break;

            case ZELDA64_DMA_UNCOMPRESSED: {
                fprintf(stdout, "zelda64: copying file %04zX\n", i);
                result = zelda64_copy_file(&out_rom, &in_rom, i);
                if (result != ZELDA64_OK) {
                    exit_code = print_error(result, "failed to copy file");
                    goto cleanup_out_rom;
                }
                break;
            }

            case ZELDA64_DMA_COMPRESSED: {
                fprintf(stdout, "zelda64: decompressing file %04zX\n", i);
                result = zelda64_decompress_file(&out_rom, &in_rom, i);
                if (result != ZELDA64_OK) {
                    exit_code = print_error(result, "failed to decompress file");
                    goto cleanup_out_rom;
                }
                break;
            }
        }
    }

    // Write out the fixed up table.
    result = zelda64_write_dmadata_to_rom(&out_rom);
    if (result != ZELDA64_OK) {
        exit_code = print_error(result, "failed to write DMADATA");
        goto cleanup_out_rom;
    }

    // Recalculate the check code and write out the header.
    result = zelda64_finalize_rom(&out_rom);
    if (result != ZELDA64_OK) {
        exit_code = print_error(result, "failed to write ROM header");
    }

cleanup_out_rom:
    zelda64_close_rom(&out_rom);
cleanup_in_rom:
    zelda64_close_rom(&in_rom);
    return exit_code;
}
