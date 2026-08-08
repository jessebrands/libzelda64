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

#include "bytes.h"
#include "crc32.h"
#include "rom.h"
#include "zelda64/zelda64.h"

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

enum zelda64_result
zelda64_read_rom_header(struct zelda64_rom_header* header, uint8_t const* data, size_t const size) {
    if (header == NULL || data == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (size < ZELDA64_ROM_HEADER_SIZE) {
        return ZELDA64_OUT_OF_RANGE;
    }
    // @formatter:off
    header->reserved0        = data[ROM_OFFSET_RESERVED0];
    memcpy(header->pi_config, &data[ROM_OFFSET_PI_CONFIG], sizeof header->pi_config);
    header->clock_rate       = zelda64_read_u32(&data[ROM_OFFSET_CLOCK_RATE]);
    header->boot_address     = zelda64_read_u32(&data[ROM_OFFSET_BOOT_ADDRESS]);
    header->libultra_version = zelda64_read_u32(&data[ROM_OFFSET_LIBULTRA_VERSION]);
    uint32_t const cc_high   = zelda64_read_u32(&data[ROM_OFFSET_CHECK_CODE]);
    uint32_t const cc_low    = zelda64_read_u32(&data[ROM_OFFSET_CHECK_CODE + 0x04]);
    header->check_code       = ((uint64_t) cc_high << 32) | ((uint64_t) cc_low);
    memcpy(header->reserved1, &data[ROM_OFFSET_RESERVED1], sizeof header->reserved1);
    memcpy(header->title,     &data[ROM_OFFSET_TITLE],     sizeof header->title);
    memcpy(header->reserved2, &data[ROM_OFFSET_RESERVED2], sizeof header->reserved2);
    memcpy(header->game_code, &data[ROM_OFFSET_GAME_CODE], sizeof header->game_code);
    header->version          = data[ROM_OFFSET_VERSION];
    // @formatter:on
    return ZELDA64_OK;
}

enum zelda64_result
zelda64_write_rom_header(uint8_t* data, size_t const size, struct zelda64_rom_header const* header) {
    if (header == NULL || data == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (size < ZELDA64_ROM_HEADER_SIZE) {
        return ZELDA64_OUT_OF_RANGE;
    }
    // @formatter:off
    data[ROM_OFFSET_RESERVED0] = header->reserved0;
    memcpy(&data[ROM_OFFSET_PI_CONFIG], header->pi_config, sizeof header->pi_config);
    zelda64_write_u32(&data[ROM_OFFSET_CLOCK_RATE],        header->clock_rate);
    zelda64_write_u32(&data[ROM_OFFSET_BOOT_ADDRESS],      header->boot_address);
    zelda64_write_u32(&data[ROM_OFFSET_LIBULTRA_VERSION],  header->libultra_version);
    zelda64_write_u32(&data[ROM_OFFSET_CHECK_CODE],        (uint32_t) (header->check_code >> 32));
    zelda64_write_u32(&data[ROM_OFFSET_CHECK_CODE + 0x04], (uint32_t) (header->check_code));
    memcpy(&data[ROM_OFFSET_RESERVED1], header->reserved1, sizeof header->reserved1);
    memcpy(&data[ROM_OFFSET_TITLE],     header->title,     sizeof header->title);
    memcpy(&data[ROM_OFFSET_RESERVED2], header->reserved2, sizeof header->reserved2);
    memcpy(&data[ROM_OFFSET_GAME_CODE], header->game_code, sizeof header->game_code);
    data[ROM_OFFSET_VERSION] = header->version;
    // @formatter:on
    return ZELDA64_OK;
}

enum zelda64_result
zelda64_read_rom_info(struct zelda64_rom_info* info, uint8_t const* data, size_t const size) {
    if (info == NULL || data == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (size < CIC_DETECT_END) {
        return ZELDA64_OUT_OF_RANGE;
    }

    *info = (struct zelda64_rom_info){0};

    enum zelda64_result const result = zelda64_read_rom_header(&info->header, data, size);
    if (result != ZELDA64_OK) {
        return result;
    }

    uint32_t const crc = zelda64_crc32(0, &data[CIC_DETECT_START], CIC_DETECT_END - CIC_DETECT_START);

    info->ipl_checksum = crc;
    info->cic = cic_for_crc32(crc);
    info->entrypoint = entry_point_for(info->header.boot_address, info->cic);

    return ZELDA64_OK;
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

static uint32_t zelda64_cic_seed(enum zelda64_cic const cic) {
    switch (cic) {
        case ZELDA64_CIC_6101:
        case ZELDA64_CIC_6102:
            return 0xF8CA4DDC;

        case ZELDA64_CIC_6103:
            return 0xA3886759;

        case ZELDA64_CIC_6105:
            return 0xDF26F436;

        case ZELDA64_CIC_6106:
            return 0x1FEA617A;

        case ZELDA64_CIC_UNKNOWN:
            break;
    }

    return 0;
}

enum zelda64_result
zelda64_cic_check_code_init(enum zelda64_cic const cic,
                            uint8_t const* ipl, size_t const ipl_size,
                            struct zelda64_check_code_state* state) {
    if (state == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (cic == ZELDA64_CIC_6105 && (ipl == NULL || ipl_size < 0x0810)) {
        return ZELDA64_INVALID_PARAMETER;
    }

    // Get initial seed for the CIC-NUS chip.
    uint32_t const seed = zelda64_cic_seed(cic);
    if (seed == 0) {
        return ZELDA64_INVALID_PARAMETER;
    }

    *state = (struct zelda64_check_code_state){
        .cic = cic,
        .acc = {seed, seed, seed, seed, seed, seed},
        .offset = 0x1000,
        .ipl = ipl,
        .ipl_size = ipl_size
    };

    return ZELDA64_OK;
}

static inline uint32_t rotate32(uint32_t const a, unsigned const b) {
    return a << b | a >> ((32 - b) & 31);
}

enum zelda64_result
zelda64_cic_check_code(struct zelda64_check_code_state* state,
                       uint8_t const* data, size_t const size) {
    // If our arguments don't make sense, bail.
    if (state == NULL || data == NULL || size % 4 != 0) {
        return ZELDA64_INVALID_PARAMETER;
    }

    // If we're dealing with a CIC-NUS-6105, we'll need the MAKEROM.
    if (state->cic == ZELDA64_CIC_6105
        && (state->ipl == NULL || state->ipl_size < 0x0810)) {
        return ZELDA64_INVALID_PARAMETER;
    }

    size_t const start = state->offset;
    size_t i = 0; // Position in current data buffer.
    while (state->offset < 0x101000 && i + 4 <= size) {
        uint32_t const d = zelda64_read_u32(&data[i]);

        if (state->acc[5] + d < state->acc[5]) {
            state->acc[3]++;
        }

        state->acc[5] += d;
        state->acc[2] ^= d;

        uint32_t const r = rotate32(d, (d & 0x1F));
        state->acc[4] += r;

        if (state->acc[1] > d) {
            state->acc[1] ^= r;
        } else {
            state->acc[1] ^= state->acc[5] ^ d;
        }

        if (state->cic == ZELDA64_CIC_6105) {
            size_t const makerom_offset = 0x0710 + ((start + i) & 0xFF);
            uint32_t const b = zelda64_read_u32(&state->ipl[makerom_offset]);
            state->acc[0] += b ^ d;
        } else {
            state->acc[0] += state->acc[4] ^ d;
        }

        i += 4;
        state->offset += 4;
    }

    return ZELDA64_OK;
}

uint64_t
zelda64_cic_check_code_end(struct zelda64_check_code_state const* state) {
    switch (state->cic) {
        case ZELDA64_CIC_6103:
            return ((uint64_t) ((state->acc[5] ^ state->acc[3]) + state->acc[2]) << 32)
                   | ((uint64_t) ((state->acc[4] ^ state->acc[1]) + state->acc[0]));


        case ZELDA64_CIC_6106:
            return ((uint64_t) ((state->acc[5] * state->acc[3]) + state->acc[2]) << 32)
                   | ((uint64_t) ((state->acc[4] * state->acc[1]) + state->acc[0]));

        default:
            return ((uint64_t) (state->acc[5] ^ state->acc[3] ^ state->acc[2]) << 32)
                   | ((uint64_t) (state->acc[4] ^ state->acc[1] ^ state->acc[0]));
    }
}

enum zelda64_result
zelda64_rom_check_code(uint8_t const* rom, size_t const rom_size, uint64_t* check_code) {
    if (rom == NULL || rom_size < 0x101000 || check_code == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    *check_code = 0;

    // Calculate the CIC-NUS chip used by the ROM (should always be 6105)
    uint32_t const crc = zelda64_crc32(0, &rom[CIC_DETECT_START], CIC_DETECT_END - CIC_DETECT_START);
    enum zelda64_cic const cic = cic_for_crc32(crc);

    uint8_t const* ipl = &rom[ZELDA64_ROM_HEADER_SIZE];
    size_t const ipl_size = 0x0FC0;

    // Initialize check code algorithm state.
    struct zelda64_check_code_state state = {0};
    enum zelda64_result result = zelda64_cic_check_code_init(cic, ipl, ipl_size, &state);
    if (result != ZELDA64_OK) {
        return result;
    }

    result = zelda64_cic_check_code(&state, &rom[state.offset], 0x101000 - state.offset);
    if (result != ZELDA64_OK) {
        return result;
    }

    *check_code = zelda64_cic_check_code_end(&state);
    return ZELDA64_OK;
}
