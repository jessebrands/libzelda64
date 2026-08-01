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

#include <stddef.h>
#include <stdint.h>

#include "zelda64/zelda64.h"

typedef void (*zelda64_io_destroy_func) (void* opaque);

struct zelda64_io_state {
    struct zelda64_allocator allocator;
    zelda64_io_destroy_func close;
};

static inline enum zelda64_result
zelda64_io_read(struct zelda64_io const* io, size_t const offset, uint8_t* dst, size_t const size) {
    if (io == NULL || (dst == NULL && size > 0)) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (io->read == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    return io->read(io->opaque, offset, dst, size);
}

static inline enum zelda64_result
zelda64_io_write(struct zelda64_io* io, size_t const offset, uint8_t const* src, size_t const size) {
    if (io == NULL || (src == NULL && size > 0)) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (io->write == NULL) {
        return ZELDA64_IO_READ_ONLY;
    }
    return io->write(io->opaque, offset, src, size);
}

static inline enum zelda64_result
zelda64_io_size(struct zelda64_io const* io, size_t* size) {
    if (io == NULL || size == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (io->size == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    return io->size(io->opaque, size);
}

static inline enum zelda64_result
zelda64_io_resize(struct zelda64_io* io, size_t size) {
    if (io == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }
    if (io->resize == NULL) {
        return io->write != NULL
        ? ZELDA64_IO_FIXED_SIZE
        : ZELDA64_IO_READ_ONLY;
    }
    return io->resize(io->opaque, size);
}

#endif //ZELDA64_IO_H
