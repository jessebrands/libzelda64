/*
 * rom.h: Nintendo 64 ROM related constants
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

#ifndef ZELDA64_ROM_H
#define ZELDA64_ROM_H

/*
 * Offsets in the IPL3 MAKEROM for specific values.
 */
#define ROM_OFFSET_RESERVED0           0x00
#define ROM_OFFSET_PI_CONFIG           0x01
#define ROM_OFFSET_CLOCK_RATE          0x04
#define ROM_OFFSET_BOOT_ADDRESS        0x08
#define ROM_OFFSET_LIBULTRA_VERSION    0x0C
#define ROM_OFFSET_CHECK_CODE          0x10
#define ROM_OFFSET_RESERVED1           0x18
#define ROM_OFFSET_TITLE               0x20
#define ROM_OFFSET_RESERVED2           0x34
#define ROM_OFFSET_GAME_CODE           0x3B
#define ROM_OFFSET_VERSION             0x3F
#define ROM_OFFSET_BOOTCODE            0x40

#endif //ZELDA64_ROM_H
