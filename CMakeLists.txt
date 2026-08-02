cmake_minimum_required(VERSION 3.21)
project(zelda64
        VERSION 0.1.0
        DESCRIPTION "Nintendo 64 Zelda ROM manipulation library"
        HOMEPAGE_URL "https://github.com/jessebrands/zelda64"
        LANGUAGES C)

option(ZELDA64_BUILD_APPLICATION "Enable building the zelda64 binary" ${PROJECT_IS_TOP_LEVEL})
option(ZELDA64_BUILD_SHARED "Enable building libzelda64 as shared library" ON)
option(ZELDA64_BUILD_STATIC "Enable building libzelda64 as static library" ON)
option(ZELDA64_BUILD_TESTS "Enable building the testing suite" ${PROJECT_IS_TOP_LEVEL})
option(ZELDA64_USE_LIBC "Enable functions that use the C standard library" ON)

mark_as_advanced(ZELDA64_USE_LIBC)

find_package(yaz0 REQUIRED)

set(ZELDA64_PUBLIC_HEADERS
        include/zelda64/zelda64.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/zelda64/config.h)

set(ZELDA64_PRIVATE_HEADERS
        lib/allocator.h
        lib/crc32.h
        lib/io.h)

set(ZELDA64_SOURCES
        lib/crc32.c
        lib/io.c
        lib/io_buffer.c
        lib/zelda64.c)

if (ZELDA64_USE_LIBC)
    list(APPEND ZELDA64_SOURCES
            lib/allocator.c)

    if (WIN32)
        list(APPEND ZELDA64_SOURCES lib/io_file_win32.c)
    elseif (UNIX)
        list(APPEND ZELDA64_SOURCES lib/io_file_posix.c)
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
            C_STANDARD 99
            C_STANDARD_REQUIRED ON
            C_EXTENSIONS OFF
            C_VISIBILITY_PRESET hidden
            VERSION ${PROJECT_VERSION}
            SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

    if (MSVC)
        target_compile_options(zelda64 PRIVATE /W4)
    else ()
        target_compile_options(zelda64
                PRIVATE -Wall -Wextra
                PRIVATE -Wconversion
                PRIVATE -Wstrict-prototypes
                PRIVATE -Wold-style-definition)
    endif ()
endif ()

if (ZELDA64_BUILD_APPLICATION OR ZELDA64_BUILD_STATIC OR ZELDA64_BUILD_TESTS)
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
            DEBUG_POSTFIX "d"
            C_STANDARD 99
            C_STANDARD_REQUIRED ON
            C_EXTENSIONS OFF)

    if (MSVC)
        target_compile_options(zelda64_static PRIVATE /W4)
    else ()
        target_compile_options(zelda64_static
                PRIVATE -Wall -Wextra
                PRIVATE -Wconversion
                PRIVATE -Wstrict-prototypes
                PRIVATE -Wold-style-definition)
    endif ()
endif ()

set(ZELDA64_BIN_HEADERS
        src/options.h)

set(ZELDA64_BIN_SOURCES
        src/main.c
        src/options.c)

if (ZELDA64_BUILD_APPLICATION AND ZELDA64_USE_LIBC)
    add_executable(zelda64_bin
            ${ZELDA64_BIN_SOURCES}
            ${ZELDA64_BIN_HEADERS})

    target_include_directories(zelda64_bin PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    set_target_properties(zelda64_bin PROPERTIES
            OUTPUT_NAME "zelda64"
            C_STANDARD 99
            C_STANDARD_REQUIRED ON
            C_EXTENSIONS OFF)

    target_link_libraries(zelda64_bin PRIVATE zelda64::zelda64_static)

    if (MSVC)
        target_compile_options(zelda64_bin PRIVATE /W4)
    else ()
        target_compile_options(zelda64_bin
                PRIVATE -Wall -Wextra
                PRIVATE -Wconversion
                PRIVATE -Wstrict-prototypes
                PRIVATE -Wold-style-definition)
    endif ()
endif ()

if (ZELDA64_BUILD_TESTS)
    enable_testing()
    add_subdirectory("test")
endif ()
