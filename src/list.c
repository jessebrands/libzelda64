/*
 * list.c: List DMADATA entries in a ROM
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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "zelda64.h"

static char const*
kind_string(enum zelda64_dma_kind const kind) {
    switch (kind) {
        case ZELDA64_DMA_EMPTY: return "empty";
        case ZELDA64_DMA_DELETED: return "deleted";
        case ZELDA64_DMA_UNCOMPRESSED: return "uncompressed";
        case ZELDA64_DMA_COMPRESSED: return "compressed";
    }
}

enum zelda64_result
list_dmadata(uint8_t const* rom, size_t const rom_size, bool verbose) {
    // Find the DMADATA location and size.
    struct zelda64_dmadata_info dmadata_info = {0};
    enum zelda64_result result = zelda64_find_dmadata(&dmadata_info, rom, rom_size);
    if (result != ZELDA64_OK) {
        return result;
    }

    // Allocate a buffer large enough to hold the DMADATA in memory.
    struct zelda64_dma_entry* entries = calloc(dmadata_info.count, sizeof(*entries));
    if (entries == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    uint8_t const* dmadata = &rom[dmadata_info.offset];
    size_t const dmadata_size = dmadata_info.size;
    result = zelda64_read_dmadata(entries, dmadata_info.count, dmadata, dmadata_size);
    if (result != ZELDA64_OK) {
        goto cleanup_entries;
    }

    // Read the ROM info now that we have the MAKEROM
    struct zelda64_rom_info rom_info;
    uint8_t const* makerom = &rom[entries[0].vrom_start];
    size_t const makerom_size = entries[0].vrom_end - entries[0].vrom_start;
    result = zelda64_read_rom_info(&rom_info, makerom, makerom_size);
    if (result != ZELDA64_OK) {
        goto cleanup_entries;
    }

    // Iterate over the entries in the list.
    int file_type[] = {
        0, 0, 0, 0,
    };


    fprintf(stdout, "      %-8s  %-8s  %-8s  %-8s  %-8s\n", "vstart", "vend", "start", "end", "kind");
    for (size_t i = 0; i < dmadata_info.count; ++i) {
        enum zelda64_dma_kind const kind = zelda64_dma_entry_kind(&entries[i]);

        fprintf(stdout, "%04" PRIXMAX"  %08X  %08X  %08X  %08X  %s\n", i,
                entries[i].vrom_start, entries[i].vrom_end,
                entries[i].rom_start, entries[i].rom_end,
                kind_string(kind));

        file_type[kind]++;
    }

    fprintf(stdout, "\n");


    if (verbose) {
        fprintf(stdout, "%zu entries: %d compressed, %d uncompressed, %d empty, %d deleted\n\n",
                dmadata_info.count,
                file_type[ZELDA64_DMA_COMPRESSED],
                file_type[ZELDA64_DMA_UNCOMPRESSED],
                file_type[ZELDA64_DMA_EMPTY],
                file_type[ZELDA64_DMA_DELETED]);
    }


cleanup_entries:
    free(entries);
    return result;
}
