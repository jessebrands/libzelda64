/*
 * rom.h: Nintendo 64 ROM handling
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

#ifndef LIBZELDA64_ROM_H
#define LIBZELDA64_ROM_H

#include <stdio.h>

#include <zelda64/zelda64.h>

struct zelda64_rom {
    struct zelda64_rom_info info;
    struct zelda64_dmadata_info dma_info;
    struct zelda64_dma_entry* dma;
};

enum zelda64_result
zelda64_open_rom(struct zelda64_rom* rom, FILE* file);

void
zelda64_close_rom(struct zelda64_rom* rom);

#endif //LIBZELDA64_ROM_H
