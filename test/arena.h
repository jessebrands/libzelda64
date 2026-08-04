/*
 * arena.h: test allocator
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

#ifndef ZELDA64_TEST_ARENA_H
#define ZELDA64_TEST_ARENA_H

#include <stddef.h>
#include <stdint.h>

#include "zelda64/zelda64.h"

#define ARENA_ALIGN ((size_t) 8)

struct arena {
    uint8_t* base;
    size_t size;
    size_t used;
    size_t live;
};

static inline void* arena_alloc(void* opaque, size_t const size) {
    struct arena* a = opaque;

    a->used = (a->used + (ARENA_ALIGN - 1u)) & ~(ARENA_ALIGN - 1u);
    if (a->used > a->size || a->size - a->used < size) {
        return NULL;
    }

    uint8_t* ptr = &a->base[a->used];
    a->used += size;
    a->live++;
    return ptr;
}

static inline void arena_free(void* opaque, void* ptr) {
    (void) ptr;

    struct arena* a = opaque;
    a->live--;
}

static inline struct zelda64_allocator arena_allocator(struct arena* a) {
    return (struct zelda64_allocator){
        .opaque = a,
        .alloc = arena_alloc,
        .free = arena_free,
    };
}

#endif //ZELDA64_TEST_ARENA_H
