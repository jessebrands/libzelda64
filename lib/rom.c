/*
 * rom.c: Nintendo 64 ROM decode functions
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

#include <string.h>

#include "crc32.h"
#include "io.h"
#include "rom.h"
#include "zelda64/zelda64.h"


#define CHUNK_SIZE          0x200
#define CIC_DETECT_START    0x40
#define CIC_DETECT_END      0x1000

static enum zelda64_cic
cic_for_crc32(uint32_t const bootcode_crc32) {
    switch (bootcode_crc32) {
        case 0x6170A4A1: return ZELDA64_CIC_6101;
        case 0x90BB6CB5: return ZELDA64_CIC_6102;
        case 0x0B050EE0: return ZELDA64_CIC_6103;
        case 0x98BC2C86: return ZELDA64_CIC_6105;
        case 0xACC8580A: return ZELDA64_CIC_6106;
        default:
            return ZELDA64_CIC_UNKNOWN;
    }
}

static uint32_t
entry_point_for(uint32_t const boot_address, enum zelda64_cic const cic) {
    switch (cic) {
        case ZELDA64_CIC_6103: return boot_address - 0x100000u;
        case ZELDA64_CIC_6106: return boot_address - 0x200000u;

        case ZELDA64_CIC_UNKNOWN:
        case ZELDA64_CIC_6101:
        case ZELDA64_CIC_6102:
        case ZELDA64_CIC_6105:
            break;
    }
    return boot_address;
}

static void
decode_header(struct zelda64_rom_header* header, uint8_t const* chunk) {
    // @formatter:off
    header->reserved0        = chunk[ROM_OFFSET_RESERVED0];
    memcpy(header->pi_config, &chunk[ROM_OFFSET_PI_CONFIG], sizeof header->pi_config);
    header->clock_rate       = zelda64_read_u32(&chunk[ROM_OFFSET_CLOCK_RATE]);
    header->boot_address     = zelda64_read_u32(&chunk[ROM_OFFSET_BOOT_ADDRESS]);
    header->libultra_version = zelda64_read_u32(&chunk[ROM_OFFSET_LIBULTRA_VERSION]);
    header->check_code       = zelda64_read_u64(&chunk[ROM_OFFSET_CHECK_CODE]);
    memcpy(header->reserved1, &chunk[ROM_OFFSET_RESERVED1], sizeof header->reserved1);
    memcpy(header->title,     &chunk[ROM_OFFSET_TITLE],     sizeof header->title);
    memcpy(header->reserved2, &chunk[ROM_OFFSET_RESERVED2], sizeof header->reserved2);
    memcpy(header->game_code, &chunk[ROM_OFFSET_GAME_CODE], sizeof header->game_code);
    header->version          = chunk[ROM_OFFSET_VERSION];
    // @formatter:on
}

enum zelda64_result
zelda64_read_rom_info(struct zelda64_io const* rom, struct zelda64_rom_info* info) {
    if (info == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    *info = (struct zelda64_rom_info){0};

    uint8_t chunk[CHUNK_SIZE];
    enum zelda64_result result = zelda64_io_read(rom, 0, chunk, ROM_HEADER_SIZE);
    if (result != ZELDA64_OK) {
        return result;
    }

    decode_header(&info->header, chunk);
    uint32_t const magic = zelda64_read_u32(chunk);

    uint32_t crc = 0;
    size_t offset = CIC_DETECT_START;
    while (offset < CIC_DETECT_END) {
        size_t const remaining = CIC_DETECT_END - offset;
        size_t const want = remaining < sizeof chunk ? remaining : sizeof chunk;

        result = zelda64_io_read(rom, offset, chunk, want);
        if (result != ZELDA64_OK) {
            return result;
        }

        crc = zelda64_crc32(crc, chunk, want);
        offset += want;
    }

    info->ipl_checksum = crc;
    info->cic = cic_for_crc32(crc);
    info->entrypoint = entry_point_for(info->header.boot_address, info->cic);

    return magic == ROM_MAGIC
               ? ZELDA64_OK
               : ZELDA64_BAD_HEADER;
}

char const*
zelda64_cic_name(enum zelda64_cic const cic) {
    switch (cic) {
        case ZELDA64_CIC_UNKNOWN: return "unknown";
        case ZELDA64_CIC_6101: return "CIC-NUS-6101";
        case ZELDA64_CIC_6102: return "CIC-NUS-6102/7101";
        case ZELDA64_CIC_6103: return "CIC-NUS-6103/7103";
        case ZELDA64_CIC_6105: return "CIC-NUS-6105/7105";
        case ZELDA64_CIC_6106: return "CIC-NUS-6106/7106";
    }
    return "unknown";
}
