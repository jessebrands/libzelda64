/*
 * dma.c: DMA table reading and writing
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of libzelda64.
 *
 * libzelda64 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libzelda64 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libzelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdbool.h>

#include "io.h"
#include "zelda64/zelda64.h"

#define CHUNK_SIZE 512u
#define DMA_ENTRY_SIZE 0x10u

static struct zelda64_dma_entry
read_entry_unsafe(uint8_t const* data) {
    return (struct zelda64_dma_entry){
        .vrom_start = zelda64_read_u32(&data[0]),
        .vrom_end = zelda64_read_u32(&data[4]),
        .rom_start = zelda64_read_u32(&data[8]),
        .rom_end = zelda64_read_u32(&data[12])
    };
}

static bool
can_be_makerom_entry(struct zelda64_dma_entry const entry) {
    return entry.vrom_start == 0 && entry.vrom_end != 0
           && entry.rom_start == 0 && entry.rom_end == 0;
}

enum zelda64_result
zelda64_find_dma_table(struct zelda64_io const* rom, struct zelda64_dma_table* table) {
    assert(rom != NULL);

    // let A = size of MAKEROM
    // let B = size of BOOTCODE
    // let C = size of DMADATA
    //
    //           vrom_start  vrom_end    rom_start   rom_end       filename
    //     0x00  0x00000000  A           0x00000000  0x00000000    MAKEROM
    //     0x01  A           A+B         A           0x00000000    BOOTCODE
    //     0x02  A+B         A+B+C       A+B         0x00000000    DMADATA
    //
    // DMADATA offset        =   A+B
    // DMADATA size in bytes =  (A+B+C) - (A+B)
    // DMADATA entry count   = ((A+B+C) - (A+B)) / 16

    if (table == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    uint8_t chunk[CHUNK_SIZE];
    size_t offset = 0;
    size_t rom_size;

    *table = (struct zelda64_dma_table){0};

    enum zelda64_result result = zelda64_io_size(rom, &rom_size);
    if (result != ZELDA64_OK) {
        return result;
    }

    while (true) {
        if (offset >= rom_size) {
            return ZELDA64_NO_DMADATA;
        }

        size_t const remaining = rom_size - offset;
        size_t const want = remaining < sizeof chunk ? remaining : sizeof chunk;

        result = zelda64_io_read(rom, offset, chunk, want);
        if (result != ZELDA64_OK) {
            return result;
        }

        for (size_t i = 0; i + DMA_ENTRY_SIZE <= want; i += DMA_ENTRY_SIZE) {
            struct zelda64_dma_entry const e0 = read_entry_unsafe(&chunk[i]);
            if (!can_be_makerom_entry(e0)) {
                continue;
            }

            // The signature at `i` could be a MAKEROM entry.
            size_t const match_pos = offset + i;
            size_t const e1_pos = match_pos + DMA_ENTRY_SIZE;
            uint8_t data[DMA_ENTRY_SIZE * 2];

            result = zelda64_io_read(rom, e1_pos, data, sizeof data);
            if (result != ZELDA64_OK) {
                return result == ZELDA64_IO_END_OF_FILE
                           ? ZELDA64_NO_DMADATA
                           : result;
            }

            struct zelda64_dma_entry const e1 = read_entry_unsafe(&data[0]);
            struct zelda64_dma_entry const e2 = read_entry_unsafe(&data[DMA_ENTRY_SIZE]);

            bool const e1_is_bootcode = e1.vrom_start == e0.vrom_end
                                        && e1.rom_start == e0.vrom_end
                                        && e1.rom_end == 0;

            bool const e2_is_dmadata = e2.vrom_start == e1.vrom_end
                                       && e2.rom_start == e1.vrom_end
                                       && e2.rom_end == 0
                                       && e2.vrom_start == match_pos;

            if (!e1_is_bootcode || !e2_is_dmadata) {
                continue;
            }

            // Check to make sure that end > start and that the range is
            // aligned to 16 bytes, as an extra safety check.
            uint32_t const span = e2.vrom_end - e2.vrom_start;
            if (e2.vrom_end > rom_size || e2.vrom_end <= e2.vrom_start || span % DMA_ENTRY_SIZE != 0) {
                continue;
            }

            // With certainty, we can say this is the DMADATA entry.
            *table = (struct zelda64_dma_table){
                .offset = e2.vrom_start,
                .size = span,
                .count = span / DMA_ENTRY_SIZE,
            };

            return ZELDA64_OK;
        }

        offset += want;
    }
}

enum zelda64_result zelda64_read_dma_table(struct zelda64_io const* rom,
                                           struct zelda64_dma_table const* table,
                                           struct zelda64_dma_entry* entries,
                                           size_t const count) {
    assert(rom != NULL);

    if (table == NULL || entries == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (count < table->count) {
        return ZELDA64_OUT_OF_RANGE;
    }

    size_t const total = table->count * DMA_ENTRY_SIZE;
    size_t bytes_in = 0;
    uint8_t chunk[CHUNK_SIZE];

    while (bytes_in < total) {
        size_t const remaining = total - bytes_in;
        size_t const want = remaining < sizeof chunk ? remaining : sizeof chunk;
        size_t const cursor = table->offset + bytes_in;

        enum zelda64_result const result = zelda64_io_read(rom, cursor, chunk, want);
        if (result != ZELDA64_OK) {
            return result;
        }

        for (size_t i = 0; i < want; i += DMA_ENTRY_SIZE) {
            size_t const entry = (bytes_in + i) / DMA_ENTRY_SIZE;
            entries[entry] = read_entry_unsafe(&chunk[i]);
        }

        bytes_in += want;
    }

    return ZELDA64_OK;
}

enum zelda64_dma_kind zelda64_dma_entry_kind(struct zelda64_dma_entry const* entry) {
    assert(entry != NULL);

    if (entry->vrom_end == entry->vrom_start) {
        return ZELDA64_DMA_EMPTY;
    }
    if (entry->rom_end == UINT32_MAX) {
        return ZELDA64_DMA_DELETED;
    }
    if (entry->rom_end == 0) {
        return ZELDA64_DMA_UNCOMPRESSED;
    }
    return ZELDA64_DMA_COMPRESSED;
}
