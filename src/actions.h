/*
 * actions.h: Program actions
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

#ifndef ZELDA64_ACTIONS_H
#define ZELDA64_ACTIONS_H

#include <stddef.h>
#include <stdint.h>

enum zelda64_action_result {
    ACTION_OK = ZELDA64_OK,
    ACTION_INVALID_PARAMETER = ZELDA64_INVALID_PARAMETER,
    ACTION_OUT_OF_RANGE = ZELDA64_OUT_OF_RANGE,
    ACTION_NO_DMADATA = ZELDA64_NO_DMADATA,
    ACTION_MEMORY_ERROR = -100,
    ACTION_IO_ERROR = 101,
};

enum zelda64_action_result
action_rom_info(uint8_t const* rom, size_t rom_size);

#endif //ZELDA64_ACTIONS_H
