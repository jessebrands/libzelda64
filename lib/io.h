/*
 * io.h: input/output abstraction
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

#ifndef ZELDA64_IO_H
#define ZELDA64_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zelda64/zelda64.h"

typedef void (*zelda64_io_destroy_func) (void* opaque);

struct zelda64_io_state {
    struct zelda64_allocator allocator;
    zelda64_io_destroy_func close;
};

struct zelda64_io_buffer {
    struct zelda64_io_state state;
    uint8_t* data;
    size_t size;
};

#endif //ZELDA64_IO_H
