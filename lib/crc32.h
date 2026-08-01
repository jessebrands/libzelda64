/*
 * crc32.h: CRC-32/IEEE checksum
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

#ifndef ZELDA64_CRC32_H
#define ZELDA64_CRC32_H

#include <stddef.h>
#include <stdint.h>

/*
 * Calculates CRC32/IEEE checksum.
 */
uint32_t zelda64_crc32(uint32_t crc, uint8_t const* data, size_t size);

#endif //ZELDA64_CRC32_H
