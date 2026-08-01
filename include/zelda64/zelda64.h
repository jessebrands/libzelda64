/*
 * zelda64.h: Nintendo 64 Zelda ROM manipulation library
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

#ifndef ZELDA64_ZELDA64_H
#define ZELDA64_ZELDA64_H

#include "zelda64/config.h"

#include <stddef.h>

#if defined _WIN32 || defined __CYGWIN__
#  if defined ZELDA64_STATIC
#    define ZELDA64_API
#  elif defined ZELDA64_SHARED
#    define ZELDA64_API __declspec(dllexport)
#  else
#    define ZELDA64_API __declspec(dllimport)
#  endif
#elif defined __GNUC__ && !defined ZELDA64_STATIC
#  define ZELDA64_API __attribute__((visibility("default")))
#else
#  define ZELDA64_API
#endif

#if defined __cplusplus
extern "C" {
#endif

typedef void* (* zelda64_alloc_func)(void* opaque, size_t size);

typedef void (* zelda64_free_func)(void* opaque, void* ptr);

enum zelda64_result {
    ZELDA64_OK = 0,
    ZELDA64_INVALID_PARAMETER = -1,
    ZELDA64_MEMORY_ERROR = -2,
    ZELDA64_IO_ERROR = -3,
    ZELDA64_OUT_OF_RANGE = -4,
};

struct zelda64_allocator {
    void* opaque;
    zelda64_alloc_func alloc;
    zelda64_free_func free;
};

ZELDA64_API char const* zelda64_result_string(enum zelda64_result result);


// !! ====================================================================== !!
//    Functions that require the C standard library go in this block.
// !! ====================================================================== !!
#if defined ZELDA64_USE_LIBC

/*
 * Creates an allocator that uses malloc/free.
 */
ZELDA64_API struct zelda64_allocator zelda64_default_allocator(void);

#endif // defined ZELDA64_USE_LIBC


#if defined __cplusplus
}
#endif

#endif //ZELDA64_ZELDA64_H
