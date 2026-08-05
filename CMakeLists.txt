cmake_minimum_required(VERSION 3.21)
project(libzelda64
        VERSION 0.1.0
        DESCRIPTION "Nintendo 64 Zelda ROM library"
        HOMEPAGE_URL "https://github.com/jessebrands/zelda64"
        LANGUAGES C)

#
# Project wide CMake settings
#
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

#
# Library settings
#
option(ZELDA64_BUILD_SHARED "Enable building libzelda64 as shared library" ON)
option(ZELDA64_BUILD_STATIC "Enable building libzelda64 as static library" ON)

#
# Utility settings
#
option(ZELDA64_BUILD_UTILITIES "Enable building utility binaries" ON)

set(ZELDA64_PUBLIC_HEADERS
        include/zelda64/zelda64.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/zelda64/config.h)

set(ZELDA64_PRIVATE_HEADERS
        src/crc32.h
        src/rom.h)

set(ZELDA64_SOURCES
        src/crc32.c
        src/dma.c
        src/rom.c
        src/zelda64.c)

function(zelda64_enable_warnings target)
    if (MSVC)
        target_compile_options(${target}
                PRIVATE /W4
                PRIVATE /w14242    # -Wconversion
                PRIVATE /w14254    # narrower bit-field conversion
                PRIVATE /w14255    # -Wstrict-prototypes
                PRIVATE /w14296    # unsigned compares
                PRIVATE /w14311    # pointer truncation
                PRIVATE /w14826)   # sign-extended conversion
    else ()
        target_compile_options(${target}
                PRIVATE -Wall -Wextra -pedantic
                PRIVATE -Wconversion
                PRIVATE -Wstrict-prototypes -Wmissing-prototypes
                PRIVATE -Wold-style-definition)
    endif ()
endfunction()

configure_file(
        include/zelda64/config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/zelda64/config.h
        @ONLY)

if (ZELDA64_BUILD_SHARED)
    add_library(zelda64 SHARED
            ${ZELDA64_SOURCES}
            ${ZELDA64_PUBLIC_HEADERS}
            ${ZELDA64_PRIVATE_HEADERS})

    add_library(zelda64::zelda64 ALIAS zelda64)

    target_include_directories(zelda64
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

    target_compile_definitions(zelda64 PRIVATE ZELDA64_SHARED)

    set_target_properties(zelda64 PROPERTIES
            OUTPUT_NAME "zelda64"
            DEBUG_POSTFIX "d"
            C_VISIBILITY_PRESET hidden
            VERSION ${PROJECT_VERSION}
            SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

    zelda64_enable_warnings(zelda64)
endif ()

if (ZELDA64_BUILD_STATIC OR ZELDA64_BUILD_UTILITIES)
    add_library(zelda64_static STATIC
            ${ZELDA64_SOURCES}
            ${ZELDA64_PUBLIC_HEADERS}
            ${ZELDA64_PRIVATE_HEADERS})

    add_library(zelda64::zelda64_static ALIAS zelda64_static)

    target_include_directories(zelda64_static
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

    target_compile_definitions(zelda64_static PUBLIC ZELDA64_STATIC)

    set_target_properties(zelda64_static PROPERTIES
            OUTPUT_NAME "zelda64s"
            DEBUG_POSTFIX "d")

    zelda64_enable_warnings(zelda64_static)
endif ()

if (ZELDA64_BUILD_UTILITIES)
    add_subdirectory("utils")
endif ()
