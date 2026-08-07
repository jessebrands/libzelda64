/*
 * zelda64.h: Program routines
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

#ifndef ZELDA64_ZELDA64_H
#define ZELDA64_ZELDA64_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zelda64/zelda64.h>

#define ZELDA64_MAX_ROM_SIZE 0x04000000u

enum zelda64_result
display_rom_info(uint8_t const* rom, size_t rom_size);

enum zelda64_result
list_dmadata(uint8_t const* rom, size_t rom_size, bool verbose);

enum zelda64_result
open_rom(char const* filename, uint8_t** rom, size_t* rom_size);

#endif //ZELDA64_ZELDA64_H
