cmake_minimum_required(VERSION 3.26)
project(zelda64
        VERSION 0.1.0
        DESCRIPTION "Nintendo 64 Zelda ROM manipulation library"
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
option(ZELDA64_BUILD_TESTS "Enable building the testing suite" ${PROJECT_IS_TOP_LEVEL})

#
# Python module settings
#
option(ZELDA64_BUILD_PYTHON_MODULE "Build the Python 3.11 module" OFF)

#
# Advanced settings
#
option(ZELDA64_USE_LIBC "Enable functions that use the C standard library" ON)

mark_as_advanced(ZELDA64_USE_LIBC)

find_package(yaz0 REQUIRED)

set(ZELDA64_PUBLIC_HEADERS
        include/zelda64/zelda64.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/zelda64/config.h)

set(ZELDA64_PRIVATE_HEADERS
        src/allocator.h
        src/crc32.h
        src/io.h
        src/rom.h)

set(ZELDA64_SOURCES
        src/crc32.c
        src/dma.c
        src/io.c
        src/io_buffer.c
        src/rom.c
        src/zelda64.c)

function(zelda64_enable_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else ()
        target_compile_options(${target}
                PRIVATE -Wall -Wextra
                PRIVATE -Wconversion
                PRIVATE -Wstrict-prototypes -Wmissing-prototypes
                PRIVATE -Wold-style-definition)
    endif ()
endfunction()

if (ZELDA64_USE_LIBC)
    list(APPEND ZELDA64_SOURCES
            src/allocator.c)

    if (WIN32)
        list(APPEND ZELDA64_SOURCES src/io_file_win32.c)
    elseif (UNIX)
        list(APPEND ZELDA64_SOURCES src/io_file_posix.c)
    else ()
        message(WARNING "No I/O and allocator implementation for this platform, turning ZELDA64_USE_LIBC off.")
        set(ZELDA64_USE_LIBC OFF)
    endif ()
endif ()

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

    target_link_libraries(zelda64
            PRIVATE yaz0::yaz0)

    target_compile_definitions(zelda64 PRIVATE ZELDA64_SHARED)

    set_target_properties(zelda64 PROPERTIES
            OUTPUT_NAME "zelda64"
            DEBUG_POSTFIX "d"
            C_VISIBILITY_PRESET hidden
            VERSION ${PROJECT_VERSION}
            SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

    zelda64_enable_warnings(zelda64)
endif ()

if (ZELDA64_BUILD_STATIC OR ZELDA64_BUILD_TESTS)
    add_library(zelda64_static STATIC
            ${ZELDA64_SOURCES}
            ${ZELDA64_PUBLIC_HEADERS}
            ${ZELDA64_PRIVATE_HEADERS})

    add_library(zelda64::zelda64_static ALIAS zelda64_static)

    target_include_directories(zelda64_static
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

    target_link_libraries(zelda64_static
            PRIVATE yaz0::yaz0_static)

    target_compile_definitions(zelda64_static PUBLIC ZELDA64_STATIC)

    set_target_properties(zelda64_static PROPERTIES
            OUTPUT_NAME "zelda64s"
            DEBUG_POSTFIX "d")

    zelda64_enable_warnings(zelda64_static)
endif ()

if (ZELDA64_BUILD_PYTHON_MODULE)
    add_subdirectory(python)
endif ()

if (ZELDA64_BUILD_TESTS)
    enable_testing()
    add_subdirectory("test")
endif ()
