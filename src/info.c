/*
 * info.h: ROM info
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
#include <stdio.h>

#include "info.h"

enum zelda64_result zelda64_run_info(struct zelda64_options const* options) {
    struct zelda64_io rom_file;
    enum zelda64_result result = zelda64_io_open_readonly(
        &rom_file,
        options->rom_filename,
        zelda64_default_allocator()
    );

    if (result != ZELDA64_OK) {
        fprintf(stderr, "Could not open '%s': %s\n",
            options->rom_filename,
            zelda64_result_string(result));

        return result;
    }

    // Read the ROM info.
    struct zelda64_rom_info info;
    result = zelda64_read_rom_info(&rom_file, &info);
    zelda64_io_close(&rom_file);

    if (result != ZELDA64_OK) {
        fprintf(stderr, "Could not read ROM header: ");
        if (result == ZELDA64_OUT_OF_RANGE) {
            fprintf(stderr, "ROM is too small\n");
        } else {
            fprintf(stderr, "%s\n", zelda64_result_string(result));
        }

        return result;
    }

    // @formatter:off
    fprintf(stdout, "%-15s%.20s\n",            "Title:", info.header.title);
    fprintf(stdout, "%-15s%.4s\n",             "Game code:", info.header.game_code);
    fprintf(stdout, "%-15s0x%02"  PRIX8  "\n", "Version:", info.header.version);
    fprintf(stdout, "%-15s%s\n",               "CIC:", zelda64_cic_name(info.cic));
    fprintf(stdout, "%-15s0x%016" PRIX64 "\n", "Check code:", info.header.check_code);
    fprintf(stdout, "%-15s0x%08"  PRIX32 "\n", "Boot address:", info.header.boot_address);
    fprintf(stdout, "%-15s0x%08"  PRIX32 "\n", "Entrypoint:", info.entrypoint);
    fprintf(stdout, "%-15s0x%08"  PRIX32 "\n", "Clock rate:", info.header.clock_rate);
    fprintf(stdout, "%-15s0x%08"  PRIX32 "\n", "libultra:", info.header.libultra_version);
    // @formatter:on

    return result;
}
