/*
 * test_dma.c: DMA table tests
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

#include <stdint.h>
#include <stdio.h>

#include "arena.h"
#include "check.h"

#define ARENA_SIZE   4096
#define FIXTURE_MAX  0x800u
#define ENTRY_SIZE   0x10u

static union {
    uint8_t bytes[ARENA_SIZE];
    void* p;
    double d;
} storage;

struct dma_fixture {
    uint8_t data[FIXTURE_MAX];
    size_t size;
    size_t table_offset;
    size_t count;
};

static void
put_u32(uint8_t* p, uint32_t const value) {
    p[0] = (uint8_t) (value >> 24);
    p[1] = (uint8_t) (value >> 16);
    p[2] = (uint8_t) (value >> 8);
    p[3] = (uint8_t) (value);
}

/*
 * Swaps half words in a fixture. This matches ROMs with .v64 extension.
 */
static void swap_halfwords(struct dma_fixture* f) {
    for (size_t i = 0; i + 1 < f->size; i += 2) {
        uint8_t const t = f->data[i];
        f->data[i] = f->data[i + 1];
        f->data[i + 1] = t;
    }
}

/*
 * Swaps words in a fixture, this matches ROMs with a .n64 extension.
 */
static void swap_words(struct dma_fixture* f) {
    for (size_t i = 0; i + 3 < f->size; i += 4) {
        uint8_t t;
        t = f->data[i];
        f->data[i] = f->data[i + 3];
        f->data[i + 3] = t;
        t = f->data[i + 1];
        f->data[i + 1] = f->data[i + 2];
        f->data[i + 2] = t;
    }
}

/*
 * Sets an entry in a DMA table.
 */
static void
fixture_set(struct dma_fixture* f, size_t const index,
            uint32_t const vs, uint32_t const ve,
            uint32_t const rs, uint32_t const re) {
    uint8_t* const p = &f->data[f->table_offset + index * ENTRY_SIZE];
    put_u32(&p[0], vs);
    put_u32(&p[4], ve);
    put_u32(&p[8], rs);
    put_u32(&p[12], re);
}

/*
 * let A = size of MAKEROM
 * let B = size of BOOTCODE
 * let C = size of DMADATA
 *
 *           vrom_start  vrom_end    rom_start   rom_end       filename
 *     0x00  0x00000000  A           0x00000000  0x00000000    MAKEROM
 *     0x01  A           A+B         A           0x00000000    BOOTCODE
 *     0x02  A+B         A+B+C       A+B         0x00000000    DMADATA
 *
 * DMADATA offset        =   A+B
 * DMADATA size in bytes =  (A+B+C) - (A+B)
 * DMADATA entry count   = ((A+B+C) - (A+B)) / 16
 */

/*
 * Initializes a fixture with a synthetic, good DMA table.
 */
static void
fixture_init(struct dma_fixture* f, uint32_t const makerom,
             uint32_t const boot, size_t const count) {
    *f = (struct dma_fixture){0};
    f->table_offset = (size_t) makerom + boot;
    f->count = count;
    f->size = f->table_offset + count * ENTRY_SIZE;

    // @formatter:off
    fixture_set(f, 0, 0,              makerom,            0,              0);
    fixture_set(f, 1, makerom,        makerom + boot,     makerom,        0);
    fixture_set(f, 2, makerom + boot, (uint32_t) f->size, makerom + boot, 0);
    // @formatter:on
}

/*
 * Test helper.
 */
static void
reject(char const* what, struct zelda64_io* io) {
    struct zelda64_dma_table table;
    check(what, zelda64_find_dma_table(io, &table), ZELDA64_NO_DMADATA);
}

static void
test_valid_dma_table(struct dma_fixture* f, struct zelda64_io* io, struct arena* a) {
    (void) a;
    (void) f;

    struct zelda64_dma_table table;

    check("  zelda64_find_dma_table", zelda64_find_dma_table(io, &table), ZELDA64_OK);
    check_that("  offset is correct", table.offset == 0x30);
    check_that("  count is correct", table.count == 50);
    check_that("  size is correct", table.size == 50 * ENTRY_SIZE);

    // Set some extra entries that match different kinds:
    fixture_set(f, 3, 0x1337, 0x1337, 0x0, 0x0); // EMPTY
    fixture_set(f, 4, 0x50, 0x60, 0x50, UINT32_MAX); // DELETED
    fixture_set(f, 5, 0x60, 0x93, 0x60, 0x80); // COMPRESSED

    struct zelda64_dma_entry e[50];
    check("  zelda64_read_dma_table", zelda64_read_dma_table(io, &table, e, sizeof e), ZELDA64_OK);

    uint32_t offset = 0;
    uint32_t size = 0;

    check_that("  e2 is uncompressed", zelda64_dma_entry_kind(&e[2]) == ZELDA64_DMA_UNCOMPRESSED);
    check("  e2 extent", zelda64_dma_entry_extent(&e[2], &offset, &size), ZELDA64_OK);
    check_that("  e2 offset correct", offset == 0x30);
    check_that("  e2 size correct", size == f->size - 0x30);

    check_that("  e3 is empty", zelda64_dma_entry_kind(&e[3]) == ZELDA64_DMA_EMPTY);
    check("  e3 extent", zelda64_dma_entry_extent(&e[3], &offset, &size), ZELDA64_OK);
    check_that("  e3 offset correct", offset == 0);
    check_that("  e3 size correct", size == 0);

    check_that("  e4 is deleted", zelda64_dma_entry_kind(&e[4]) == ZELDA64_DMA_DELETED);
    check("  e4 extent is invalid", zelda64_dma_entry_extent(&e[4], &offset, &size), ZELDA64_INVALID_PARAMETER);

    check_that("  e5 is compressed", zelda64_dma_entry_kind(&e[5]) == ZELDA64_DMA_COMPRESSED);
    check("  e5 extent", zelda64_dma_entry_extent(&e[5], &offset, &size), ZELDA64_OK);
    check_that("  e5 offset correct", offset == 0x60);
    check_that("  e5 size correct", size == 0x20);
}

static void
test_invalid_dma_table0(struct dma_fixture* f, struct zelda64_io* io, struct arena* a) {
    (void) a;

    // Zeroing out the third entry means the algorithm will never find it.
    fixture_set(f, 2, 0, 0, 0, 0);
    reject("  reports no DMADATA", io);
}

static void
test_invalid_dma_table1(struct dma_fixture* f, struct zelda64_io* io, struct arena* a) {
    (void) a;

    fixture_set(f, 0, 0x0000, 0x0010, 0x0000, 0x0000);
    fixture_set(f, 1, 0x0020, 0x0030, 0x0020, 0x0000); // Does not match expected pattern.

    reject("  reports no DMADATA", io);
}

static void
test_invalid_dma_table2(struct dma_fixture* f, struct zelda64_io* io, struct arena* a) {
    (void) a;

    // Compressed files can never match.
    fixture_set(f, 0, 0x0000, 0x0010, 0x0000, 0x0000);
    fixture_set(f, 1, 0x0010, 0x0030, 0x0010, 0x0000);
    fixture_set(f, 2, 0x0030, f->size, 0x0030, 0x0064);

    reject("  reports no DMADATA", io);
}

static void
test_dma_table_beyond_bounds_fails(struct dma_fixture* f, struct zelda64_io* io, struct arena* a) {
    (void) a;

    fixture_set(f, 2, 0x0030, f->size * 2, 0x0030, 0x0000);
    reject("  reports no DMADATA", io);
}

static void
test_halfword_swapped_dma_table_is_rejected(struct dma_fixture* f, struct zelda64_io* io, struct arena* a) {
    (void) a;

    swap_halfwords(f);
    reject("  is not found", io);
}

static void
test_little_endian_dma_table_is_rejected(struct dma_fixture* f, struct zelda64_io* io, struct arena* a) {
    (void) a;
    (void) io;

    swap_words(f);
    reject("  is not found", io);
}

static void
test_empty_is_invalid_param(struct dma_fixture* f, struct zelda64_io* io, struct arena* a) {
    (void) f;
    (void) a;
    (void) io;

    check("  is invalid parameter", zelda64_find_dma_table(io, NULL), ZELDA64_INVALID_PARAMETER);
}

typedef void (* test_func)(struct dma_fixture* f, struct zelda64_io* io, struct arena* a);

static void
do_test(char const* name, test_func const t) {
    struct arena a = {storage.bytes, sizeof storage.bytes, 0, 0};
    struct dma_fixture f = {0};
    struct zelda64_io io = {0};
    fixture_init(&f, 0x10, 0x20, 50);

    printf("\n%s\n", name);
    if (zelda64_io_from_const_buffer(&io, f.data, f.size, arena_allocator(&a)) != ZELDA64_OK) {
        printf("  == TEST ERROR ==\n");
        return;
    }

    t(&f, &io, &a);
    zelda64_io_close(&io);

    check_that("  all memory released", a.live == 0);
}

int main(void) {
    do_test("big endian dma table is found", test_valid_dma_table);
    do_test("null argument is invalid parameter", test_empty_is_invalid_param);
    do_test("little endian dma table is not found", test_little_endian_dma_table_is_rejected);
    do_test("halfword swapped dma table is not found", test_halfword_swapped_dma_table_is_rejected);
    do_test("invalid dma table is rejected", test_invalid_dma_table0);
    do_test("dma table rejected if e0.vs != e1.ve", test_invalid_dma_table1);
    do_test("dma table rejected if e0 .. e2 are compressed", test_invalid_dma_table2);
    do_test("dma table rejected if entry 2 is bigger than rom", test_dma_table_beyond_bounds_fails);
    return check_report();
}
