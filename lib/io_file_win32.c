/*
 * io_file_win32.c: Windows file abstraction
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "allocator.h"
#include "io.h"

struct zelda64_io_win32_file {
    struct zelda64_io_state state;
    HANDLE handle;
};

static void zelda64_io_win32_file_close(void* opaque) {
    struct zelda64_io_win32_file const* file = opaque;
    CloseHandle(file->handle);
}

static enum zelda64_result
zelda64_io_win32_file_read(void* opaque, size_t const offset, uint8_t* dst, size_t const size) {
    if (size > MAXDWORD) {
        return ZELDA64_OUT_OF_RANGE;
    }

    struct zelda64_io_win32_file const* file = opaque;

    OVERLAPPED ov;
    ZeroMemory(&ov, sizeof(ov));
    ov.Offset = (DWORD) (offset & 0xFFFFFFFFu);
    ov.OffsetHigh = (DWORD) (offset >> 32);

    DWORD want = (DWORD) size;
    DWORD bytes_in;
    if (!ReadFile(file->handle, dst, want, &bytes_in, &ov)) {
        return GetLastError() == ERROR_HANDLE_EOF
                   ? ZELDA64_OUT_OF_RANGE
                   : ZELDA64_IO_ERROR;
    }

    return want == bytes_in
               ? ZELDA64_OK
               : ZELDA64_OUT_OF_RANGE;
}

static enum zelda64_result
zelda64_io_win32_file_write(void* opaque, size_t const offset, uint8_t const* src, size_t const size) {
    if (size > MAXDWORD) {
        return ZELDA64_OUT_OF_RANGE;
    }

    struct zelda64_io_win32_file const* file = opaque;

    OVERLAPPED ov;
    ZeroMemory(&ov, sizeof(ov));
    ov.Offset = (DWORD) (offset & 0xFFFFFFFFu);
    ov.OffsetHigh = (DWORD) (offset >> 32);

    DWORD have = (DWORD) size;
    DWORD bytes_out;
    if (!WriteFile(file->handle, src, have, &bytes_out, &ov)) {
        return ZELDA64_IO_ERROR;
    }

    return have == bytes_out
               ? ZELDA64_OK
               : ZELDA64_IO_ERROR;
}

static enum zelda64_result
zelda64_io_win32_file_size(void* opaque, size_t* size) {
    struct zelda64_io_win32_file const* file = opaque;

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file->handle, &file_size)) {
        return ZELDA64_IO_ERROR;
    }

    // This matters for 32-bit builds.
    if (file_size.QuadPart < 0 || (unsigned long long) file_size.QuadPart > SIZE_MAX) {
        return ZELDA64_OUT_OF_RANGE;
    }

    *size = (size_t) file_size.QuadPart;
    return ZELDA64_OK;
}

static enum zelda64_result
zelda64_io_win32_file_resize(void* opaque, size_t const size) {
    struct zelda64_io_win32_file const* file = opaque;

    LARGE_INTEGER file_size;
    file_size.QuadPart = (LONGLONG) size;

    if (!SetFilePointerEx(file->handle, file_size, NULL, FILE_BEGIN)) {
        return ZELDA64_IO_ERROR;
    }

    return SetEndOfFile(file->handle)
               ? ZELDA64_OK
               : ZELDA64_IO_ERROR;
}

static enum zelda64_result
zelda64_widen_path(char const* path, struct zelda64_allocator const allocator, wchar_t** out) {
    int const length = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        path, -1, NULL, 0
    );

    if (length <= 0) {
        return ZELDA64_INVALID_PARAMETER;
    }

    wchar_t* wide = zelda64_alloc(allocator, (size_t) length * sizeof(wchar_t));
    if (wide == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                            path, -1, wide, length) == 0) {
        zelda64_free(allocator, wide);
        return ZELDA64_INVALID_PARAMETER;
    }

    *out = wide;
    return ZELDA64_OK;
}

enum zelda64_result zelda64_io_open(struct zelda64_io* io, char const* filename,
                                    struct zelda64_allocator const allocator) {
    if (!zelda64_allocator_valid(allocator) || io == NULL || filename == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    struct zelda64_io_win32_file* file = zelda64_alloc(allocator, sizeof(*file));
    if (file == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    wchar_t* wide_path = NULL;
    enum zelda64_result const result = zelda64_widen_path(filename, allocator, &wide_path);
    if (result != ZELDA64_OK) {
        zelda64_free(allocator, file);
        return result;
    }

    file->handle = CreateFileW(
        wide_path,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    zelda64_free(allocator, wide_path);

    if (file->handle == INVALID_HANDLE_VALUE) {
        zelda64_free(allocator, file);
        return ZELDA64_IO_ERROR;
    }

    file->state.allocator = allocator;
    file->state.close = zelda64_io_win32_file_close;

    *io = (struct zelda64_io){
        .read = zelda64_io_win32_file_read,
        .write = zelda64_io_win32_file_write,
        .size = zelda64_io_win32_file_size,
        .resize = zelda64_io_win32_file_resize,
        .opaque = file,
    };

    return ZELDA64_OK;
}

enum zelda64_result zelda64_io_open_readonly(struct zelda64_io* io, char const* filename,
                                             struct zelda64_allocator const allocator) {
    if (!zelda64_allocator_valid(allocator) || io == NULL || filename == NULL) {
        return ZELDA64_INVALID_PARAMETER;
    }

    struct zelda64_io_win32_file* file = zelda64_alloc(allocator, sizeof(*file));
    if (file == NULL) {
        return ZELDA64_MEMORY_ERROR;
    }

    wchar_t* wide_path = NULL;
    enum zelda64_result const result = zelda64_widen_path(filename, allocator, &wide_path);
    if (result != ZELDA64_OK) {
        zelda64_free(allocator, file);
        return result;
    }

    file->handle = CreateFileW(
        wide_path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    zelda64_free(allocator, wide_path);

    if (file->handle == INVALID_HANDLE_VALUE) {
        zelda64_free(allocator, file);
        return ZELDA64_IO_ERROR;
    }

    file->state.allocator = allocator;
    file->state.close = zelda64_io_win32_file_close;

    *io = (struct zelda64_io){
        .read = zelda64_io_win32_file_read,
        .write = NULL,
        .size = zelda64_io_win32_file_size,
        .resize = NULL,
        .opaque = file,
    };

    return ZELDA64_OK;
}
