/*
 * info.c: Print Nintendo 64 ROM information
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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <zelda64/zelda64.h>

#include "zelda64.h"

enum zelda64_result
display_rom_info(uint8_t const* rom, size_t const rom_size) {
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

    fprintf(stdout, "%-20s%.20s\n", "title:", rom_info.header.title);
    fprintf(stdout, "%-20s%.4s\n", "game code:", rom_info.header.game_code);
    fprintf(stdout, "%-20s%d\n", "version", rom_info.header.version + 1);
    fprintf(stdout, "%-20s0x%016" PRIX64 "\n", "check code:", rom_info.header.check_code);
    fprintf(stdout, "%-20s%s\n", "cic:", zelda64_cic_name(rom_info.cic));
    fprintf(stdout, "%-20s0x%08" PRIX32 "\n", "ipl checksum:", rom_info.ipl_checksum);
    fprintf(stdout, "%-20s0x%08" PRIX32 "\n", "boot address:", rom_info.header.boot_address);
    fprintf(stdout, "%-20s0x%08" PRIX32 "\n", "entrypoint:", rom_info.entrypoint);
    fprintf(stdout, "%-20s0x%" PRIX32 "\n", "libultra:", rom_info.header.libultra_version);
    fprintf(stdout, "%-20s0x%08" PRIX32"\n", "dmadata offset:", dmadata_info.offset);
    fprintf(stdout, "%-20s%zu\n", "dmadata entries:", dmadata_info.count);
    fprintf(stdout, "\n");

cleanup_entries:
    free(entries);
    return result;
}
