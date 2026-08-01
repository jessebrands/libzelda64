/*
 * io_buffer.c: in-memory I/O buffer
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

#include <string.h>

#include "allocator.h"
#include "io.h"

struct zelda64_io_buffer {
    struct zelda64_io_state state;
    uint8_t* data;
    size_t size;
};

static void zelda64_io_close_buffer(void* opaque) {
    struct zelda64_io_buffer const* buffer = opaque;
    if (buffer->data != NULL) {
        zelda64_free(buffer->state.allocator, buffer->data);
    }
}

static enum zelda64_result
zelda64_io_buffer_read(void* opaque, size_t const offset, uint8_t* dst, size_t const size) {
    struct zelda64_io_buffer const* buffer = opaque;
    if (offset > buffer->size || size > buffer->size - offset) {
        return ZELDA64_OUT_OF_RANGE;
    }
    // memcpy with a NULL source is undefined behavior
    if (size == 0) {
        return ZELDA64_OK;
    }
    memcpy(dst, &buffer->data[offset], size);
    return ZELDA64_OK;
}

static enum zelda64_result
zelda64_io_buffer_write(void* opaque, size_t const offset, uint8_t const* src, size_t const size) {
    struct zelda64_io_buffer const* buffer = opaque;
    if (offset > buffer->size || size > buffer->size - offset) {
        return ZELDA64_OUT_OF_RANGE;
    }
    // memcpy with a NULL source is undefined behavior
    if (size == 0) {
        return ZELDA64_OK;
    }
    memcpy(&buffer->data[offset], src, size);
    return ZELDA64_OK;
}

static enum zelda64_result
zelda64_io_buffer_size(void* opaque, size_t* size) {
    struct zelda64_io_buffer const* buffer = opaque;
    *size = buffer->size;
    return ZELDA64_OK;
}

static enum zelda64_result
zelda64_io_buffer_resize(void* opaque, size_t const size) {
    struct zelda64_io_buffer* buffer = opaque;
    if (size == buffer->size) {
        return ZELDA64_OK;
    }

    // An empty file is a zero length buffer in memory, so this is valid.
    if (size == 0) {
        zelda64_io_close_buffer(buffer);
        buffer->data = NULL;
        buffer->size = 0;
        return ZELDA64_OK;
    }

    // Attempt the resize first, don't corrupt the buffer if we fail.
    uint8_t* old_buffer = buffer->data;
    size_t const old_size = size < buffer->size ? size : buffer->size;
    uint8_t* new_buffer = zelda64_alloc(buffer->state.allocator, size);

    if (new_buffer == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    // Copy over the old buffer and zero the new region.
    if (old_size > 0) {
        memcpy(new_buffer, old_buffer, old_size);
    }
    size_t const extra_bytes = size - old_size;
    if (extra_bytes > 0) {
        memset(&new_buffer[old_size], 0, extra_bytes);
    }

    if (buffer->data != NULL) {
        zelda64_free(buffer->state.allocator, old_buffer);
    }

    buffer->data = new_buffer;
    buffer->size = size;
    return ZELDA64_OK;
}

enum zelda64_result
zelda64_io_create_buffer(struct zelda64_io* io, size_t const size, struct zelda64_allocator const allocator) {
    if (!zelda64_allocator_valid(allocator) || io == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    struct zelda64_io_buffer* buffer = zelda64_alloc(allocator, sizeof(*buffer));
    if (buffer == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    // Do not reject size == 0, `io_buffer` acts like a file so zero length
    // files need to be representable and this is how we do that.
    if (size > 0) {
        buffer->data = zelda64_alloc(allocator, size);
        if (buffer->data == NULL) {
            zelda64_free(allocator, buffer);
            return ZELDA64_MEMORY_ERROR;
        }
        memset(buffer->data, 0, size);
    } else {
        buffer->data = NULL;
    }

    buffer->size = size;
    buffer->state.allocator = allocator;
    buffer->state.close = zelda64_io_close_buffer;

    *io = (struct zelda64_io){
        .read = zelda64_io_buffer_read,
        .write = zelda64_io_buffer_write,
        .size = zelda64_io_buffer_size,
        .resize = zelda64_io_buffer_resize,
        .opaque = buffer,
    };

    return ZELDA64_OK;
}

enum zelda64_result
zelda64_io_from_buffer(struct zelda64_io* io, uint8_t* data, size_t const size,
                       struct zelda64_allocator const allocator) {
    if (!zelda64_allocator_valid(allocator) || io == NULL || (data == NULL && size > 0)) {
        return ZELDA64_INVALID_PARAMETER;
    }

    struct zelda64_io_buffer* buffer = zelda64_alloc(allocator, sizeof(*buffer));
    if (buffer == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    buffer->data = data;
    buffer->size = size;
    buffer->state.allocator = allocator;
    buffer->state.close = NULL;

    *io = (struct zelda64_io){
        .read = zelda64_io_buffer_read,
        .write = zelda64_io_buffer_write,
        .size = zelda64_io_buffer_size,
        .resize = NULL,
        .opaque = buffer
    };

    return ZELDA64_OK;
}

enum zelda64_result
zelda64_io_from_const_buffer(struct zelda64_io* io, uint8_t const* data, size_t const size,
                             struct zelda64_allocator const allocator) {
    if (!zelda64_allocator_valid(allocator) || io == NULL || (data == NULL && size > 0)) {
        return ZELDA64_INVALID_PARAMETER;
    }

    struct zelda64_io_buffer* buffer = zelda64_alloc(allocator, sizeof(*buffer));
    if (buffer == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    // Removing const is fine, we're setting only read functions.
    buffer->data = (uint8_t*) data;
    buffer->size = size;
    buffer->state.allocator = allocator;
    buffer->state.close = NULL;

    *io = (struct zelda64_io){
        .read = zelda64_io_buffer_read,
        .write = NULL,
        .size = zelda64_io_buffer_size,
        .resize = NULL,
        .opaque = buffer
    };

    return ZELDA64_OK;
}
