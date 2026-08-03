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

#define ZELDA64_MAKEROM_SIZE    0x1000
#define ZELDA64_ROM_HEADER_SIZE 0x40
#define ZELDA64_BOOTCODE_SIZE   0xFC0

#if defined __cplusplus
extern "C" {
#endif

enum zelda64_result {
    ZELDA64_OK = 0,
    ZELDA64_INVALID_PARAMETER = -1,
    ZELDA64_MEMORY_ERROR = -2,
    ZELDA64_OUT_OF_RANGE = -3,
    ZELDA64_BAD_HEADER = -4,
    ZELDA64_NO_DMADATA = -5,
    ZELDA64_IO_ERROR = -100,
    ZELDA64_IO_READ_ONLY = -101,
    ZELDA64_IO_FIXED_SIZE = -102,
    ZELDA64_IO_END_OF_FILE = -103,
};

enum zelda64_compression_level {
    ZELDA64_DEFAULT_COMPRESSION = -1,
    ZELDA64_NO_COMPRESSION = 0,
    ZELDA64_BEST_COMPRESSION = 9,
};

enum zelda64_cic {
    ZELDA64_CIC_UNKNOWN = 0,
    ZELDA64_CIC_6101 = 6101,
    ZELDA64_CIC_6102 = 6102,
    ZELDA64_CIC_6103 = 6103,
    ZELDA64_CIC_6105 = 6105,
    ZELDA64_CIC_6106 = 6106,
};

enum zelda64_dma_kind {
    ZELDA64_DMA_EMPTY = 0,
    ZELDA64_DMA_DELETED,
    ZELDA64_DMA_UNCOMPRESSED,
    ZELDA64_DMA_COMPRESSED,
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

struct zelda64_rom_header {
    uint8_t reserved0;          // 0x00  0x80 on all commercial ROMs
    uint8_t pi_config[3];       // 0x01  PI BSD DOM1 RLS/PGS/PWD/LAT
    uint32_t clock_rate;        // 0x04
    uint32_t boot_address;      // 0x08  not the entry point on 6103/6106
    uint32_t libultra_version;  // 0x0C
    uint64_t check_code;        // 0x10  commonly called CRC1/CRC2
    uint8_t reserved1[8];       // 0x18
    char title[20];             // 0x20  20 bytes, padded (NOT NUL-terminated)
    uint8_t reserved2[7];       // 0x34
    char game_code[4];          // 0x3B  4 bytes (NOT NUL-terminated)
    uint8_t version;            // 0x3F
};

struct zelda64_rom_info {
    struct zelda64_rom_header header;
    uint32_t ipl_checksum;
    enum zelda64_cic cic;
    uint32_t entrypoint;
};

struct zelda64_dma_table {
    uint32_t offset;  // physical offset of DMADATA in ROM
    uint32_t size;    // size in bytes
    size_t count;     // count of entries in DMADATA
};

struct zelda64_dma_entry {
    uint32_t vrom_start;
    uint32_t vrom_end;
    uint32_t rom_start;
    uint32_t rom_end;
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

ZELDA64_API enum zelda64_result
zelda64_io_read(struct zelda64_io const* io, size_t offset, uint8_t* dst, size_t size);

ZELDA64_API enum zelda64_result
zelda64_io_write(struct zelda64_io* io, size_t offset, uint8_t const* src, size_t size);

ZELDA64_API enum zelda64_result
zelda64_io_size(struct zelda64_io const* io, size_t* size);

ZELDA64_API enum zelda64_result
zelda64_io_resize(struct zelda64_io* io, size_t size);

/*
 * Reads and decodes the Nintendo 64 IPL section of a ROM.
 */
ZELDA64_API enum zelda64_result
zelda64_read_rom_info(struct zelda64_io const* rom, struct zelda64_rom_info* info);

ZELDA64_API char const*
zelda64_cic_name(enum zelda64_cic cic);

/*
 * Attempts to locate the DMA table in a ROM.
 */
ZELDA64_API enum zelda64_result
zelda64_find_dma_table(struct zelda64_io const* rom,
                       struct zelda64_dma_table* table);

ZELDA64_API enum zelda64_result
zelda64_read_dma_table(struct zelda64_io const* rom,
                       struct zelda64_dma_table const* table,
                       struct zelda64_dma_entry* entries,
                       size_t count);

ZELDA64_API enum zelda64_dma_kind
zelda64_dma_entry_kind(struct zelda64_dma_entry const* entry);

ZELDA64_API enum zelda64_result
zelda64_dma_entry_extent(struct zelda64_dma_entry const* entry,
                         uint32_t* offset, uint32_t* size);


// !! ====================================================================== !!
//    Functions that require the C standard library go in this block.
// !! ====================================================================== !!
#if defined ZELDA64_USE_LIBC

/*
 * Creates an allocator that uses malloc/free.
 */
ZELDA64_API struct zelda64_allocator zelda64_default_allocator(void);

/*
 * Opens a file for reading and writing.
 */
ZELDA64_API enum zelda64_result
zelda64_io_open(struct zelda64_io* io,
                char const* filename,
                struct zelda64_allocator allocator);

/*
 * Opens a file for reading only.
 */
ZELDA64_API enum zelda64_result
zelda64_io_open_readonly(struct zelda64_io* io,
                         char const* filename,
                         struct zelda64_allocator allocator);

#endif // defined ZELDA64_USE_LIBC


#if defined __cplusplus
}
#endif

#endif //ZELDA64_ZELDA64_H
