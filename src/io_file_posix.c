/*
 * io_file_posix.c: POSIX file abstraction
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

#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64

#include "allocator.h"
#include "io.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

struct zelda64_io_posix_file {
    struct zelda64_io_state state;
    int fd;
};

static void zelda64_io_posix_file_close(void* opaque) {
    struct zelda64_io_posix_file const* file = opaque;
    close(file->fd);
}

static enum zelda64_result
zelda64_io_posix_file_read(void* opaque, size_t const offset, uint8_t* dst, size_t const size) {
    struct zelda64_io_posix_file const* file = opaque;

    size_t bytes_in = 0;
    while (bytes_in < size) {
        ssize_t const got = pread(file->fd, &dst[bytes_in], size - bytes_in, (off_t) (offset + bytes_in));
        if (got < 0) {
            // Interrupt signals are not a failure state.
            if (errno == EINTR) {
                continue;
            }

            return ZELDA64_IO_ERROR;
        }

        // If nothing went out, we've hit EOF.
        if (got == 0) {
            return ZELDA64_IO_END_OF_FILE;
        }

        bytes_in += (size_t) got;
    }

    return ZELDA64_OK;
}

static enum zelda64_result
zelda64_io_posix_file_write(void* opaque, size_t const offset, uint8_t const* src, size_t const size) {
    struct zelda64_io_posix_file const* file = opaque;

    size_t bytes_out = 0;
    while (bytes_out < size) {
        ssize_t const put = pwrite(file->fd, &src[bytes_out], size - bytes_out,
                                   (off_t) (offset + bytes_out));
        if (put < 0) {
            // Interrupt signals are not a failure state.
            if (errno == EINTR) {
                continue;
            }

            return ZELDA64_IO_ERROR;
        }

        // Nothing going out is a problem...
        if (put == 0) {
            return ZELDA64_IO_ERROR;
        }

        bytes_out += (size_t) put;
    }

    return ZELDA64_OK;
}

static enum zelda64_result
zelda64_io_posix_file_size(void* opaque, size_t* size) {
    struct zelda64_io_posix_file const* file = opaque;

    struct stat stat;
    if (fstat(file->fd, &stat) < 0) {
        return errno == EOVERFLOW
                   ? ZELDA64_OUT_OF_RANGE
                   : ZELDA64_IO_ERROR;
    }

    if (stat.st_size < 0 || (unsigned long long) stat.st_size > SIZE_MAX) {
        return ZELDA64_OUT_OF_RANGE;
    }

    *size = (size_t) stat.st_size;
    return ZELDA64_OK;
}

static enum zelda64_result
zelda64_io_posix_file_resize(void* opaque, size_t const size) {
    struct zelda64_io_posix_file const* file = opaque;

    if (ftruncate(file->fd, (off_t) size) < 0) {
        return errno == EFBIG || errno == EINVAL
                   ? ZELDA64_OUT_OF_RANGE
                   : ZELDA64_IO_ERROR;
    }

    return ZELDA64_OK;
}

enum zelda64_result zelda64_io_open(struct zelda64_io* io, char const* filename,
                                    struct zelda64_allocator const allocator) {
    if (!zelda64_allocator_valid(allocator) || io == NULL || filename == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    struct zelda64_io_posix_file* file = zelda64_alloc(allocator, sizeof(*file));
    if (file == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    file->fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
    if (file->fd < 0) {
        zelda64_free(allocator, file);
        return ZELDA64_IO_ERROR;
    }

    file->state.allocator = allocator;
    file->state.close = zelda64_io_posix_file_close;

    *io = (struct zelda64_io){
        .read = zelda64_io_posix_file_read,
        .write = zelda64_io_posix_file_write,
        .size = zelda64_io_posix_file_size,
        .resize = zelda64_io_posix_file_resize,
        .opaque = file,
    };

    return ZELDA64_OK;
}

enum zelda64_result zelda64_io_open_readonly(struct zelda64_io* io, char const* filename,
                                             struct zelda64_allocator const allocator) {
    if (!zelda64_allocator_valid(allocator) || io == NULL || filename == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    struct zelda64_io_posix_file* file = zelda64_alloc(allocator, sizeof(*file));
    if (file == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    file->fd = open(filename, O_RDONLY | O_CLOEXEC);
    if (file->fd < 0) {
        zelda64_free(allocator, file);
        return ZELDA64_IO_ERROR;
    }

    file->state.allocator = allocator;
    file->state.close = zelda64_io_posix_file_close;

    *io = (struct zelda64_io){
        .read = zelda64_io_posix_file_read,
        .write = NULL,
        .size = zelda64_io_posix_file_size,
        .resize = NULL,
        .opaque = file,
    };

    return ZELDA64_OK;
}
