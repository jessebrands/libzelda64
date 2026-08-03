/*
 * list.c: DMA table listing
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

#include <inttypes.h>
#include <stdlib.h>

#include "commands.h"

static char const*
entry_kind_name(enum zelda64_dma_kind const kind) {
    switch (kind) {
        case ZELDA64_DMA_EMPTY: return "empty";
        case ZELDA64_DMA_DELETED: return "deleted";
        case ZELDA64_DMA_UNCOMPRESSED: return "uncompressed";
        case ZELDA64_DMA_COMPRESSED: return "compressed";
    }
    return "unknown";
}

enum zelda64_result
zelda64_list_dma_table(struct zelda64_options const* options) {
    struct zelda64_io rom_file;
    enum zelda64_result result;

    result = zelda64_io_open_readonly(
        &rom_file,
        options->rom_filename,
        zelda64_default_allocator()
    );

    if (result != ZELDA64_OK) {
        fprintf(stderr, "Could not open '%s': %s\n", options->rom_filename, zelda64_result_string(result));
        return result;
    }

    // Find the DMA table.
    struct zelda64_dma_table table;
    result = zelda64_find_dma_table(&rom_file, &table);
    if (result != ZELDA64_OK) {
        fprintf(stderr, "Could not locate DMA table: %s\n", zelda64_result_string(result));
        goto cleanup_rom;
    }

    // Load the DMA table.
    struct zelda64_dma_entry* entries = malloc(table.count * sizeof *entries);
    if (entries == NULL) {
        result = ZELDA64_MEMORY_ERROR;
        goto cleanup_rom;
    }

    result = zelda64_read_dma_table(&rom_file, &table, entries, table.count);
    if (result != ZELDA64_OK) {
        fprintf(stderr, "Could not read DMA table: %s\n", zelda64_result_string(result));
        goto cleanup_entries;
    }

    // We got what we need, so let's get to writing it out.
    fprintf(stdout, "      %-10s  %-10s  %-10s  %-10s  %s\n",
            "VROM start", "VROM end", "ROM start", "ROM end", "Kind");

    size_t counts[] = {0, 0, 0, 0};

    for (size_t i = 0; i < table.count; ++i) {
        struct zelda64_dma_entry const* entry = &entries[i];
        enum zelda64_dma_kind const kind = zelda64_dma_entry_kind(entry);

        fprintf(stdout, "%04" PRIXMAX "  0x%08X  0x%08X  0x%08X  0x%08X  %s\n",
                i,
                entry->vrom_start,
                entry->vrom_end,
                entry->rom_start,
                entry->rom_end,
                entry_kind_name(kind)
        );

        counts[kind]++;
    }

    fprintf(stdout, "\n"
            "%" PRIuMAX " entries: "
            "%" PRIuMAX " compressed, "
            "%" PRIuMAX " uncompressed, "
            "%" PRIuMAX " deleted, "
            "%" PRIuMAX " empty\n",
            table.count, counts[3], counts[2], counts[1], counts[0]);

cleanup_entries:
    free(entries);
cleanup_rom:
    zelda64_io_close(&rom_file);
    return result;
}
