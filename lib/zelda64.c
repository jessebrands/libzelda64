/*
 * zelda64.c: Nintendo 64 Zelda ROM manipulation common functions
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

#include "zelda64/zelda64.h"

char const* zelda64_result_string(enum zelda64_result const result) {
    switch (result) {
        case ZELDA64_OK: return "ok";
        case ZELDA64_INVALID_PARAMETER: return "invalid parameter";
        case ZELDA64_MEMORY_ERROR: return "memory error";
        case ZELDA64_IO_ERROR: return "i/o error";
        case ZELDA64_OUT_OF_RANGE: return "out of range";
    }
    return "unknown";
}


// !! ====================================================================== !!
//    Functions that require the C standard library go in this block.
// !! ====================================================================== !!
#if defined ZELDA64_USE_LIBC

#include <stdlib.h>

/*
 * Default memory allocation callback.
 */
static void* zelda64_malloc(void* opaque, size_t const size) {
    (void)opaque; // unused parameter
    return malloc(size);
}

/*
 * Default memory release callback.
 */
static void zelda64_free(void* opaque, void* ptr) {
    (void)opaque;
    free(ptr);
}

struct zelda64_allocator zelda64_default_allocator(void) {
    return (struct zelda64_allocator) {
        .opaque = NULL,
        .alloc = zelda64_malloc,
        .free = zelda64_free,
    };
}

#endif // defined ZELDA64_USE_LIBC
