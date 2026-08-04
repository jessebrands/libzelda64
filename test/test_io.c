/*
 * test_io.c: input/output abstraction tests
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

#include <stdio.h>
#include <string.h>

#include "zelda64/zelda64.h"
#include "io.h"

static int failures = 0;

static void check(char const* what, enum zelda64_result const got, enum zelda64_result const want) {
    int const ok = got == want;
    printf("%-42s %-20s %s\n", what, zelda64_result_string(got), ok ? "ok" : "FAIL");
    if (!ok) {
        failures += 1;
    }
}

static void check_that(char const* what, int const ok) {
    printf("%-42s %-20s %s\n", what, "", ok ? "ok" : "FAIL");
    if (!ok) {
        failures += 1;
    }
}

static void test_buffer(struct zelda64_allocator const allocator) {
    static uint8_t const fixture[4] = {0xCA, 0xFE, 0xBA, 0xBE};
    uint8_t const source[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    struct zelda64_io io = {0};
    uint8_t scratch[16];
    uint8_t caller[4] = {1, 2, 3, 4};
    size_t size = 0;

    puts("-- buffer --");
    check("create_buffer(16)", zelda64_io_create_buffer(&io, 16, allocator), ZELDA64_OK);
    check("size", zelda64_io_size(&io, &size), ZELDA64_OK);
    check_that("  size is 16", size == 16);
    check("read fresh buffer", zelda64_io_read(&io, 0, scratch, 16), ZELDA64_OK);
    check_that("  zero initialised", scratch[0] == 0 && scratch[15] == 0);

    check("write 4 at 4", zelda64_io_write(&io, 4, source, 4), ZELDA64_OK);
    check("read back at 4", zelda64_io_read(&io, 4, scratch, 4), ZELDA64_OK);
    check_that("  contents match", memcmp(scratch, source, 4) == 0);

    check("read past end", zelda64_io_read(&io, 13, scratch, 4), ZELDA64_IO_END_OF_FILE);
    check("read 0 bytes at end", zelda64_io_read(&io, 16, scratch, 0), ZELDA64_OK);
    check("read 0 bytes past end", zelda64_io_read(&io, 17, scratch, 0), ZELDA64_IO_END_OF_FILE);
    check("read NULL dst", zelda64_io_read(&io, 0, NULL, 4), ZELDA64_INVALID_PARAMETER);

    check("grow to 32", zelda64_io_resize(&io, 32), ZELDA64_OK);
    check("read old region", zelda64_io_read(&io, 4, scratch, 4), ZELDA64_OK);
    check_that("  survived resize", memcmp(scratch, source, 4) == 0);
    memset(scratch, 0xAA, sizeof scratch);
    check("read grown region", zelda64_io_read(&io, 24, scratch, 8), ZELDA64_OK);
    check_that("  zero filled", scratch[0] == 0 && scratch[7] == 0);

    check("shrink to 8", zelda64_io_resize(&io, 8), ZELDA64_OK);
    check("shrink to 0", zelda64_io_resize(&io, 0), ZELDA64_OK);
    check("grow from 0", zelda64_io_resize(&io, 16), ZELDA64_OK);
    zelda64_io_close(&io);
    check_that("  close zeroes the io", io.opaque == NULL && io.read == NULL);
    zelda64_io_close(&io);
    check_that("  close twice is safe", 1);

    check("create_buffer(0)", zelda64_io_create_buffer(&io, 0, allocator), ZELDA64_OK);
    check("read 0 from empty", zelda64_io_read(&io, 0, scratch, 0), ZELDA64_OK);
    zelda64_io_close(&io);

    check("from_buffer", zelda64_io_from_buffer(&io, caller, 4, allocator), ZELDA64_OK);
    check("  write allowed", zelda64_io_write(&io, 0, source, 4), ZELDA64_OK);
    check_that("  wrote through to caller memory", caller[0] == 0xDE);
    check("  resize refused", zelda64_io_resize(&io, 8), ZELDA64_IO_FIXED_SIZE);
    zelda64_io_close(&io);
    check_that("  close left caller memory alone", caller[0] == 0xDE && caller[3] == 0xEF);
    check("from_buffer with NULL data", zelda64_io_from_buffer(&io, NULL, 4, allocator),
          ZELDA64_INVALID_PARAMETER);

    check("from_const_buffer", zelda64_io_from_const_buffer(&io, fixture, 4, allocator),
          ZELDA64_OK);
    check("  read allowed", zelda64_io_read(&io, 0, scratch, 4), ZELDA64_OK);
    check_that("  contents match", memcmp(scratch, fixture, 4) == 0);
    check("  write refused", zelda64_io_write(&io, 0, source, 4), ZELDA64_IO_READ_ONLY);
    check("  resize refused", zelda64_io_resize(&io, 8), ZELDA64_IO_READ_ONLY);
    zelda64_io_close(&io);
}

static void test_buffer_data(struct zelda64_allocator const allocator) {
    static uint8_t const fixture[4] = {0xCA, 0xFE, 0xBA, 0xBE};
    uint8_t const source[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t caller[4] = {1, 2, 3, 4};
    struct zelda64_io io = {0};
    struct zelda64_io const zeroed = {0};
    uint8_t const* data = NULL;
    size_t size = 0;

    puts("-- buffer_data --");
    check("create_buffer(16)", zelda64_io_create_buffer(&io, 16, allocator), ZELDA64_OK);
    check("write 4 at 4", zelda64_io_write(&io, 4, source, 4), ZELDA64_OK);
    check("buffer_data", zelda64_io_buffer_data(&io, &data, &size), ZELDA64_OK);
    check_that("  size is 16", size == 16);
    check_that("  data is not NULL", data != NULL);
    check_that("  sees the written bytes", data != NULL && memcmp(&data[4], source, 4) == 0);
    check_that("  zero elsewhere", data != NULL && data[0] == 0 && data[15] == 0);

    // The old pointer is not compared against, since reading a freed pointer's
    // value is itself indeterminate; that resize reallocates is asserted by the
    // contents surviving through whatever pointer we get back.
    check("grow to 32", zelda64_io_resize(&io, 32), ZELDA64_OK);
    check("buffer_data after resize", zelda64_io_buffer_data(&io, &data, &size), ZELDA64_OK);
    check_that("  size is 32", size == 32);
    check_that("  contents survived", data != NULL && memcmp(&data[4], source, 4) == 0);

    check("buffer_data with NULL data", zelda64_io_buffer_data(&io, NULL, &size),
          ZELDA64_INVALID_PARAMETER);
    check("buffer_data with NULL size", zelda64_io_buffer_data(&io, &data, NULL),
          ZELDA64_INVALID_PARAMETER);
    check("buffer_data with NULL io", zelda64_io_buffer_data(NULL, &data, &size),
          ZELDA64_INVALID_PARAMETER);
    zelda64_io_close(&io);
    check("buffer_data on closed io", zelda64_io_buffer_data(&io, &data, &size),
          ZELDA64_INVALID_PARAMETER);
    check("buffer_data on zeroed io", zelda64_io_buffer_data(&zeroed, &data, &size),
          ZELDA64_INVALID_PARAMETER);

    // Poison both out-params so that an empty buffer reporting (NULL, 0) is
    // distinguishable from the function not having written to them at all.
    data = fixture;
    size = 99;
    check("create_buffer(0)", zelda64_io_create_buffer(&io, 0, allocator), ZELDA64_OK);
    check("buffer_data on empty buffer", zelda64_io_buffer_data(&io, &data, &size), ZELDA64_OK);
    check_that("  size is 0", size == 0);
    check_that("  data is NULL", data == NULL);
    zelda64_io_close(&io);

    check("from_const_buffer", zelda64_io_from_const_buffer(&io, fixture, 4, allocator),
          ZELDA64_OK);
    check("  buffer_data", zelda64_io_buffer_data(&io, &data, &size), ZELDA64_OK);
    check_that("    hands back the caller's pointer", data == fixture);
    check_that("    size is 4", size == 4);
    zelda64_io_close(&io);

    check("from_buffer", zelda64_io_from_buffer(&io, caller, 4, allocator), ZELDA64_OK);
    check("  buffer_data", zelda64_io_buffer_data(&io, &data, &size), ZELDA64_OK);
    check_that("    hands back the caller's pointer", data == caller);
    check_that("    size is 4", size == 4);
    zelda64_io_close(&io);
}

static void test_file(struct zelda64_allocator const allocator) {
    char const* const path = "rom_\xC3\xB8\xE6\xBC\xA2.z64";
    uint8_t const source[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
    struct zelda64_io io = {0};
    uint8_t scratch[32];
    uint8_t const* data = NULL;
    size_t size = 0;

    puts("-- file --");
    check("open with non-ascii path", zelda64_io_open(&io, path, allocator), ZELDA64_OK);
    check("size", zelda64_io_size(&io, &size), ZELDA64_OK);
    check_that("  new file is empty", size == 0);

    check("write 8 at 0", zelda64_io_write(&io, 0, source, 8), ZELDA64_OK);
    check("size", zelda64_io_size(&io, &size), ZELDA64_OK);
    check_that("  size is 8", size == 8);
    check("read back", zelda64_io_read(&io, 0, scratch, 8), ZELDA64_OK);
    check_that("  contents match", memcmp(scratch, source, 8) == 0);

    check("read past end", zelda64_io_read(&io, 100, scratch, 4), ZELDA64_IO_END_OF_FILE);
    check("read straddling end", zelda64_io_read(&io, 6, scratch, 8), ZELDA64_IO_END_OF_FILE);

    check("write past end", zelda64_io_write(&io, 16, source, 4), ZELDA64_OK);
    check("size", zelda64_io_size(&io, &size), ZELDA64_OK);
    check_that("  file was extended", size == 20);
    memset(scratch, 0xAA, sizeof scratch);
    check("read the gap", zelda64_io_read(&io, 8, scratch, 8), ZELDA64_OK);
    check_that("  gap is zero filled", scratch[0] == 0 && scratch[7] == 0);

    check("grow to 64", zelda64_io_resize(&io, 64), ZELDA64_OK);
    check("size", zelda64_io_size(&io, &size), ZELDA64_OK);
    check_that("  size is 64", size == 64);
    memset(scratch, 0xAA, sizeof scratch);
    check("read extension", zelda64_io_read(&io, 32, scratch, 32), ZELDA64_OK);
    check_that("  zero filled", scratch[0] == 0 && scratch[31] == 0);

    check("shrink to 4", zelda64_io_resize(&io, 4), ZELDA64_OK);
    check("size", zelda64_io_size(&io, &size), ZELDA64_OK);
    check_that("  size is 4", size == 4);
    zelda64_io_close(&io);
    zelda64_io_close(&io);
    check_that("  close twice is safe", 1);

    check("open_readonly", zelda64_io_open_readonly(&io, path, allocator), ZELDA64_OK);
    check("  read allowed", zelda64_io_read(&io, 0, scratch, 4), ZELDA64_OK);
    check_that("  survived round trip", memcmp(scratch, source, 4) == 0);
    check("  write refused", zelda64_io_write(&io, 0, source, 4), ZELDA64_IO_READ_ONLY);
    check("  resize refused", zelda64_io_resize(&io, 8), ZELDA64_IO_READ_ONLY);
    // The discriminator: a file io must be rejected without its opaque being
    // touched, which is what keeps buffer_data safe against foreign ios.
    check("  buffer_data refused", zelda64_io_buffer_data(&io, &data, &size),
          ZELDA64_INVALID_PARAMETER);
    zelda64_io_close(&io);

    check("open_readonly on missing file",
          zelda64_io_open_readonly(&io, "does_not_exist.z64", allocator), ZELDA64_IO_ERROR);

#if defined _WIN32
    // POSIX filepaths are just byte streams so this test makes no sense on POSIX.
    check("open with invalid utf-8", zelda64_io_open(&io, "\xFF\xFE.z64", allocator), ZELDA64_INVALID_PARAMETER);
#endif

    check("open with NULL path", zelda64_io_open(&io, NULL, allocator), ZELDA64_INVALID_PARAMETER);

    // remove() takes a narrow path, which on Windows is interpreted as ANSI so
    // the file never ends up removed; so we just let the file be, I guess. :-)
}

int main(void) {
    struct zelda64_allocator const allocator = zelda64_default_allocator();
    test_buffer(allocator);
    test_buffer_data(allocator);
    test_file(allocator);

    printf("\n%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
