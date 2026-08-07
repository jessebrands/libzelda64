/*
 * file.c: ROM file routines
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zelda64.h"

enum zelda64_result
open_rom(char const* filename, uint8_t** rom, size_t* rom_size) {
    enum zelda64_result result = ZELDA64_OK;
    *rom = NULL;
    *rom_size = 0;

    // Open file and get the file size.
    FILE* in_file = fopen(filename, "rb");
    if (in_file == NULL) {
        fprintf(stderr, "zelda64: error: cannot open '%s': %s\n", filename, strerror(errno));
        return ZELDA64_IO_ERROR;
    }

    // Read the ROM file size.
    fseek(in_file, 0, SEEK_END);
    long const in_size = ftell(in_file);
    if (in_size < 0) {
        fprintf(stderr, "zelda64: error: cannot determine the size of '%s': %s\n",
                filename, strerror(errno));
        result = ZELDA64_IO_ERROR;
        goto cleanup_file;
    }
    if (in_size == 0) {
        fprintf(stderr, "zelda64: error: '%s' is empty\n", filename);
        result = ZELDA64_OUT_OF_RANGE;
        goto cleanup_file;
    }
    if ((unsigned long) in_size > ZELDA64_MAX_ROM_SIZE) {
        fprintf(stderr, "zelda64: error: '%s' is too large to be a Nintendo 64 ROM\n", filename);
        result = ZELDA64_OUT_OF_RANGE;
        goto cleanup_file;
    }

    // Allocate a buffer large enough to hold the ROM in memory.
    uint8_t* in_rom = malloc((size_t) in_size);
    if (in_rom == NULL) {
        fprintf(stderr, "zelda64: error: out of memory\n");
        result = ZELDA64_MEMORY_ERROR;
        goto cleanup_file;
    }

    // Read the ROM to memory.
    rewind(in_file);
    if (fread(in_rom, 1, (size_t) in_size, in_file) != (size_t) in_size) {
        if (ferror(in_file)) {
            fprintf(stderr, "zelda64: error: cannot read '%s': %s\n", filename, strerror(errno));
        } else {
            fprintf(stderr, "zelda64: error: '%s' ended unexpectedly\n", filename);
        }
        result = ZELDA64_IO_ERROR;
        goto cleanup_rom;
    }

    *rom = in_rom;
    *rom_size = (size_t) in_size;
    goto cleanup_file;

cleanup_rom:
    free(in_rom);
cleanup_file:
    fclose(in_file);
    return result;
}
