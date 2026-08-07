/*
 * rom.c: Nintendo 64 ROM handling
 * Copyright (C) 2026 Jesse Gerard Brands
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
#include <stdlib.h>

#include "rom.h"

#define CHUNK_ENTRY_COUNT 256
#define CHUNK_SIZE (CHUNK_ENTRY_COUNT * ZELDA64_DMA_ENTRY_SIZE)

static enum zelda64_result
find_dma_table(FILE* f, struct zelda64_dmadata_info* info) {
    if (f == NULL || info == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    // We need the file size later for a sanity check.
    fseek(f, 0, SEEK_END);
    long int const file_size = ftell(f);
    if (file_size < 0) {
        return ZELDA64_INVALID_PARAMETER;
    }

    size_t const rom_size = (size_t) file_size;

    // Start from the beginning of the ROM
    size_t offset = 0;

    while (true) {
        uint8_t chunk[CHUNK_SIZE];

        // Load the next entries.
        fseek(f, (long int) offset, SEEK_SET);
        size_t const entries_in = fread(chunk, ZELDA64_DMA_ENTRY_SIZE, CHUNK_ENTRY_COUNT, f);
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

            // Attempt to read info about DMADATA.
            if (zelda64_read_dmadata_info(info, dma_start, rom_size, entries, sizeof entries) != ZELDA64_OK) {
                seek_pos += ZELDA64_DMA_ENTRY_SIZE; // This entry ain't it.
                continue;
            }

            return ZELDA64_OK;
        }

        offset += got;
    }
}

static enum zelda64_result
read_dma_entries(struct zelda64_dma_entry* entries, size_t const count,
                 FILE* file, size_t const offset) {
    fseek(file, (long int) offset, SEEK_SET);

    for (size_t i = 0; i < count; i += 256) {
        size_t const remaining = count - i;
        size_t const in_count = remaining < 256 ? remaining : 256;
        uint8_t chunk[CHUNK_SIZE];

        if (fread(chunk, sizeof(struct zelda64_dma_entry), in_count, file) != in_count) {
            return ZELDA64_OUT_OF_RANGE;
        }

        enum zelda64_result const result = zelda64_read_dmadata(&entries[i], in_count, chunk, CHUNK_SIZE);
        if (result != ZELDA64_OK) {
            return result;
        }
    }

    return ZELDA64_OK;
}

/*
 * Reads the ROM information from the MAKEROM
 */
static enum zelda64_result
read_rom_info(struct zelda64_rom* rom, struct zelda64_dma_entry const* makerom_entry, FILE* file) {
    uint32_t makerom_offset, makerom_size;
    enum zelda64_result result = zelda64_dma_entry_extent(makerom_entry, &makerom_offset, &makerom_size);
    if (result != ZELDA64_OK) {
        return result;
    }

    uint8_t* makerom = malloc(makerom_size);
    if (makerom == NULL) {
        return ZELDA64_OUT_OF_RANGE;
    }

    fseek(file, (long int) makerom_offset, SEEK_SET);
    if (fread(makerom, sizeof(uint8_t), makerom_size, file) != makerom_size) {
        result = ZELDA64_OUT_OF_RANGE;
        goto cleanup_makerom;
    }

    result = zelda64_read_rom_info(&rom->info, makerom, makerom_size);

cleanup_makerom:
    free(makerom);
    return result;
}


enum zelda64_result zelda64_open_rom(struct zelda64_rom* rom, FILE* file) {
    if (rom == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    *rom = (struct zelda64_rom){0};

    enum zelda64_result result = find_dma_table(file, &rom->dma_info);
    if (result != ZELDA64_OK) {
        return result;
    }

    rom->dma = calloc(rom->dma_info.count, sizeof(*rom->dma));
    if (rom->dma == NULL) {
        return ZELDA64_OUT_OF_RANGE;
    }

    result = read_dma_entries(rom->dma, rom->dma_info.count, file, rom->dma_info.offset);
    if (result != ZELDA64_OK) {
        goto cleanup_dma_table;
    }

    // Allocate a temporary buffer for the MAKEROM, which is the first entry
    // in the DMADATA.
    struct zelda64_dma_entry const* makerom_entry = &rom->dma[0];
    result = read_rom_info(rom, makerom_entry, file);
    if (result != ZELDA64_OK) {
        goto cleanup_dma_table;
    }

    return result;

cleanup_dma_table:
    free(rom->dma);
    rom->dma = NULL;
    return result;
}

void zelda64_close_rom(struct zelda64_rom* rom) {
    free(rom->dma);
    rom->dma = NULL;
}
