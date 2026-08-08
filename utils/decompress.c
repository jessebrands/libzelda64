/*
 * decompress.c: Nintendo 64 Zelda ROM Decompressor
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of zelda64.
 *
 * zelda64 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * zelda64 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with zelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yaz0/yaz0.h>
#include <zelda64/zelda64.h>

static inline void
zelda64_write_u32(uint8_t* p, uint32_t const value) {
    // @formatter:off
    p[0] = (uint8_t) (value >> 24);
    p[1] = (uint8_t) (value >> 16);
    p[2] = (uint8_t) (value >> 8);
    p[3] = (uint8_t) (value);
    // @formatter:on
}

static inline void
zelda64_write_u64(uint8_t* p, uint64_t const value) {
    zelda64_write_u32(&p[0], (uint32_t) (value >> 32));
    zelda64_write_u32(&p[4], (uint32_t) (value));
}

static bool
open_file(char const* filename, uint8_t** data, size_t* size) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "decompress: error: could not open file: %s\n", filename);
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "decompress: error: failed to read file: %s\n", filename);
        goto cleanup_file;
    }

    long const file_size = ftell(file);
    rewind(file);

    if (file_size < 0) {
        fprintf(stderr, "decompress: error: failed to get file size: %s\n", filename);
        goto cleanup_file;;
    }

    *size = (size_t) file_size;
    *data = malloc(*size);
    if (*data == NULL) {
        fprintf(stderr, "decompress: error: out of memory\n");
        goto cleanup_file;
    }

    if (fread(*data, 1, *size, file) != *size) {
        fprintf(stderr, "decompress: error: unexpected end of file: %s\n", filename);
        goto cleanup_buffer;
    }

    fclose(file);
    return true;

cleanup_buffer:
    free(*data);
cleanup_file:
    fclose(file);
    *data = NULL;
    *size = 0;
    return false;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s rom outfile\n", argv[0]);
        return EXIT_FAILURE;
    }

    char const* in_filename = argv[1];
    char const* out_filename = argv[2];

    // Open the source ROM.
    uint8_t* in_rom;
    size_t in_size;
    if (!open_file(in_filename, &in_rom, &in_size)) {
        return EXIT_FAILURE;
    }

    struct zelda64_dmadata_info dma_info;
    enum zelda64_result result = zelda64_find_dmadata(&dma_info, in_rom, in_size);
    if (result != ZELDA64_OK) {
        fprintf(stderr, "zelda64: error: could not find DMADATA: %s\n", zelda64_result_string(result));
        return EXIT_FAILURE;
    }

    struct zelda64_dma_entry* in_table = calloc(dma_info.count, sizeof *in_table);
    if (in_table == NULL) {
        fprintf(stderr, "zelda64: error: out of memory!\n");
        goto cleanup_in_rom;
    }

    result = zelda64_read_dmadata(in_table, dma_info.count,
                                  &in_rom[dma_info.offset], in_size - dma_info.offset);

    if (result != ZELDA64_OK) {
        fprintf(stderr, "zelda64: error: failed to read DMADATA: %s\n", zelda64_result_string(result));
        goto cleanup_in_table;
    }

    // Allocate an output ROM.
    uint8_t* out_rom = calloc(0x4000000, sizeof *out_rom);
    if (out_rom == NULL) {
        fprintf(stderr, "zelda64: error: out of memory!\n");
        goto cleanup_in_table;
    }

    struct zelda64_dma_entry* out_table = calloc(dma_info.count, sizeof *in_table);
    if (out_table == NULL) {
        fprintf(stderr, "zelda64: error: out of memory!\n");
        goto cleanup_out_rom;
    }

    for (size_t i = 0; i < dma_info.count; ++i) {
        struct zelda64_dma_entry const in_entry = in_table[i];
        enum zelda64_dma_kind const kind = zelda64_dma_entry_kind(&in_entry);
        struct zelda64_dma_entry out_entry = in_entry;

        switch (kind) {
            case ZELDA64_DMA_EMPTY:
                fprintf(stdout, "zelda64: skipping empty file %04zX\n", i);
                break;

            case ZELDA64_DMA_DELETED:
                fprintf(stdout, "zelda64: skipping deleted file %04zX\n", i);
                break;

            case ZELDA64_DMA_UNCOMPRESSED: {
                fprintf(stdout, "zelda64: copying file %04zX\n", i);

                uint32_t file_offset = 0;
                uint32_t file_size = 0;
                result = zelda64_dma_entry_extent(&in_entry, &file_offset, &file_size);
                if (result != ZELDA64_OK) {
                    fprintf(stderr, "zelda64: error: invalid entry: %s\n", zelda64_result_string(result));
                    goto cleanup_out_table;
                }

                memcpy(&out_rom[in_entry.vrom_start], &in_rom[file_offset], file_size);

                // Fix up the table.
                out_entry.rom_start = in_entry.vrom_start;
                out_entry.rom_end = 0x0;
                break;
            }

            case ZELDA64_DMA_COMPRESSED: {
                fprintf(stdout, "zelda64: decompressing file %04zX\n", i);

                uint32_t file_offset = 0;
                uint32_t file_size = 0;
                result = zelda64_dma_entry_extent(&in_entry, &file_offset, &file_size);
                if (result != ZELDA64_OK) {
                    fprintf(stderr, "zelda64: error: invalid entry: %s\n", zelda64_result_string(result));
                    goto cleanup_out_table;
                }

                struct yaz0_stream stream = {0};
                if (yaz0_decompress_init(&stream) != YAZ0_OK) {
                    fprintf(stderr, "zelda64: error: could not initialize decompressor\n");
                    goto cleanup_out_table;
                }

                stream.next_in = &in_rom[file_offset];
                stream.avail_in = file_size;
                stream.next_out = &out_rom[in_entry.vrom_start];
                stream.avail_out = in_entry.vrom_end - in_entry.vrom_start;

                enum yaz0_result const decomp_result = yaz0_decompress(&stream, YAZ0_FINISH);
                if (decomp_result != YAZ0_STREAM_END) {
                    fprintf(stderr, "zelda64: error: decompression failed: %s\n", yaz0_result_name(decomp_result));
                    yaz0_decompress_end(&stream);
                    goto cleanup_out_table;
                }

                yaz0_decompress_end(&stream);

                // Fix up the table.
                out_entry.rom_start = in_entry.vrom_start;
                out_entry.rom_end = 0x0;
                break;
            }
        }

        out_table[i] = out_entry;
    }

    // Write out the fixed up table.
    result = zelda64_write_dmadata(&out_rom[dma_info.offset], dma_info.size, out_table, dma_info.count);
    if (result != ZELDA64_OK) {
        fprintf(stderr, "zelda64: error: failed to write DMADATA: %s\n", zelda64_result_string(result));
        goto cleanup_out_table;
    }

    // Recalculate the check code.
    uint64_t check_code;
    result = zelda64_rom_check_code(out_rom, 0x4000000, &check_code);
    if (result != ZELDA64_OK) {
        fprintf(stderr, "zelda64: error: failed to calculate check code: %s\n", zelda64_result_string(result));
        goto cleanup_out_table;
    }

    // TODO: This needs a library routine.
    zelda64_write_u64(&out_rom[0x10], check_code);

    // Write memory resident ROM to file.
    FILE* out_file = fopen(out_filename, "wb");
    if (out_file == NULL) {
        fprintf(stderr, "zelda64: error: failed to open output ROM\n");
        goto cleanup_out_table;
    }

    if (fwrite(out_rom, 1, 0x4000000, out_file) != 0x4000000) {
        fprintf(stderr, "zelda64: error: did not write all bytes to output ROM file\n");
        goto cleanup_out_file;
    }

    fclose(out_file);
    return EXIT_SUCCESS;

cleanup_out_file:
    fclose(out_file);
cleanup_out_table:
    free(out_table);
cleanup_out_rom:
    free(out_rom);
cleanup_in_table:
    free(in_table);
cleanup_in_rom:
    free(in_rom);
    return EXIT_FAILURE;
}
