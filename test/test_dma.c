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
        f->data[i] = f->data[i + 1]; f->data[i + 1] = t;
    }
}

/*
 * Swaps words in a fixture, this matches ROMs with a .n64 extension.
 */
static void swap_words(struct dma_fixture* f) {
    for (size_t i = 0; i + 3 < f->size; i += 4) {
        uint8_t t;
        t = f->data[i];     f->data[i]     = f->data[i+3]; f->data[i+3] = t;
        t = f->data[i + 1]; f->data[i + 1] = f->data[i+2]; f->data[i+2] = t;
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
reject(char const* what, struct dma_fixture const* f, struct zelda64_allocator const alloc) {
    struct zelda64_io io = {0};
    struct zelda64_dma_table table;
    check("  from_const_buffer", zelda64_io_from_const_buffer(&io, f->data, f->size, alloc), ZELDA64_OK);
    check(what, zelda64_find_dma_table(&io, &table), ZELDA64_NO_DMADATA);
    zelda64_io_close(&io);
}

static void
test_valid_dma_table(void) {
    // Set up.
    struct arena a = {storage.bytes, sizeof storage.bytes, 0, 0};
    struct dma_fixture f = {0};
    fixture_init(&f, 0x10, 0x20, 50);

    // Test
    struct zelda64_io io = {0};
    struct zelda64_dma_table table;

    printf("\ntest_valid_dma_table\n");
    check("  from_const_buffer", zelda64_io_from_const_buffer(&io, f.data, f.size, arena_allocator(&a)), ZELDA64_OK);
    check("  zelda64_find_dma_table", zelda64_find_dma_table(&io, &table), ZELDA64_OK);
    check_that("  offset is correct", table.offset == 0x30);
    check_that("  count is correct", table.count == 50);
    check_that("  size is correct", table.size == 50 * ENTRY_SIZE);

    // Teardown
    zelda64_io_close(&io);

    check_that("  all pointers freed", a.live == 0);
}

static void
test_halfword_swapped_dma_table_is_rejected(void) {
    struct arena a = {storage.bytes, sizeof storage.bytes, 0, 0};
    struct dma_fixture f = {0};
    fixture_init(&f, 0x10, 0x20, 50);
    swap_halfwords(&f);

    // Swap the ROM
    printf("\ntest_halfword_swapped_rom_is_rejected\n");
    reject("  is not found", &f, arena_allocator(&a));
    check_that("  all pointers freed", a.live == 0);
}

static void
test_little_endian_dma_table_is_rejected(void) {
    struct arena a = {storage.bytes, sizeof storage.bytes, 0, 0};
    struct dma_fixture f = {0};
    fixture_init(&f, 0x10, 0x20, 50);
    swap_words(&f);

    // Swap the ROM
    printf("\ntest_little_endian_rom_is_rejected\n");
    reject("  is not found", &f, arena_allocator(&a));
    check_that("  all pointers freed", a.live == 0);
}

int main(void) {
    test_valid_dma_table();
    test_halfword_swapped_dma_table_is_rejected();
    test_little_endian_dma_table_is_rejected();
    return check_report();
}
