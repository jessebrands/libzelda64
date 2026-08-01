cmake_minimum_required(VERSION 3.21)
project(zelda64
        VERSION 0.1.0
        DESCRIPTION "Nintendo 64 Zelda ROM manipulation library"
        HOMEPAGE_URL "https://github.com/jessebrands/zelda64"
        LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(ZELDA64_BUILD_APPLICATION "Enable building the zelda64 binary" ${PROJECT_IS_TOP_LEVEL})
option(ZELDA64_BUILD_SHARED "Enable building libzelda64 as shared library" ON)
option(ZELDA64_BUILD_STATIC "Enable building libzelda64 as static library" ON)

if (ZELDA64_BUILD_SHARED)
    add_library(zelda64 SHARED lib/zelda64.c)
    add_library(zelda64::zelda64 ALIAS zelda64)

    target_include_directories(zelda64
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib")

    set_target_properties(zelda64 PROPERTIES
            OUTPUT_NAME "zelda64"
            DEBUG_POSTFIX "d"
            C_STANDARD 99
            C_STANDARD_REQUIRED ON
            C_EXTENSIONS OFF
            VERSION ${PROJECT_VERSION}
            SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
endif ()

if (ZELDA64_BUILD_APPLICATION OR ZELDA64_BUILD_STATIC)
    add_library(zelda64_static STATIC lib/zelda64.c)
    add_library(zelda64::zelda64_static ALIAS zelda64_static)

    target_include_directories(zelda64_static
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib")

    set_target_properties(zelda64_static PROPERTIES
            OUTPUT_NAME "zelda64s"
            DEBUG_POSTFIX "d"
            C_STANDARD 99
            C_STANDARD_REQUIRED ON
            C_EXTENSIONS OFF
            VERSION ${PROJECT_VERSION}
            SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
endif ()

if (ZELDA64_BUILD_APPLICATION)
    add_executable(zelda64_bin src/main.c)
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
        target_compile_options(zelda64_bin PRIVATE -Wall -Wextra -Wconversion)
    endif ()
endif ()
