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

#include <stddef.h>
#include <stdint.h>

#include "zelda64/config.h"

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

enum zelda64_result {
    ZELDA64_OK = 0,
    ZELDA64_INVALID_PARAMETER = -1,
    ZELDA64_MEMORY_ERROR = -2,
    ZELDA64_OUT_OF_RANGE = -3,
    ZELDA64_IO_ERROR = -100,
    ZELDA64_IO_READ_ONLY = -101,
    ZELDA64_IO_FIXED_SIZE = -102,
};

#define ZELDA64_IS_IO_ERROR(x) ((x) <= ZELDA64_IO_ERROR)

typedef void* (* zelda64_alloc_func)(void* opaque, size_t size);

typedef void (* zelda64_free_func)(void* opaque, void* ptr);

typedef enum zelda64_result (* zelda64_io_read_func)(void* opaque, size_t offset, uint8_t* dst, size_t size);

typedef enum zelda64_result (* zelda64_io_write_func)(void* opaque, size_t offset, uint8_t const* src, size_t size);

typedef enum zelda64_result (* zelda64_io_size_func)(void* opaque, size_t* size);

typedef enum zelda64_result (* zelda64_io_resize_func)(void* opaque, size_t size);

struct zelda64_allocator {
    void* opaque;
    zelda64_alloc_func alloc;
    zelda64_free_func free;
};

struct zelda64_io {
    zelda64_io_read_func read;
    zelda64_io_write_func write;
    zelda64_io_size_func size;
    zelda64_io_resize_func resize;
    void* opaque;
};

ZELDA64_API char const* zelda64_result_string(enum zelda64_result result);

/*
 * Creates an in-memory read/write file and allocates a buffer for it.
 */
ZELDA64_API enum zelda64_result
zelda64_io_create_buffer(struct zelda64_io* io,
                         size_t size,
                         struct zelda64_allocator allocator);

/*
 * Creates an in-memory read/write file from a buffer.
 */
ZELDA64_API enum zelda64_result
zelda64_io_from_buffer(struct zelda64_io* io,
                       uint8_t* data,
                       size_t size,
                       struct zelda64_allocator allocator);

/*
 * Creates an in-memory read-only file from a buffer.
 */
ZELDA64_API enum zelda64_result
zelda64_io_from_const_buffer(struct zelda64_io* io,
                             uint8_t const* data,
                             size_t size,
                             struct zelda64_allocator allocator);

/*
 * Closes an zelda64_io file, releasing its resources.
 */
ZELDA64_API void zelda64_io_close(struct zelda64_io* io);


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
