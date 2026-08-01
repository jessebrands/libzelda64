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

if (ZELDA64_BUILD_APPLICATION)
    add_executable(zelda64_bin src/main.c)
    target_include_directories(zelda64_bin PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    set_target_properties(zelda64_bin PROPERTIES
            OUTPUT_NAME "zelda64"
            C_STANDARD 99
            C_STANDARD_REQUIRED ON
            C_EXTENSIONS OFF)

    if (MSVC)
        target_compile_options(zelda64_bin PRIVATE /W4)
    else ()
        target_compile_options(zelda64_bin PRIVATE -Wall -Wextra -Wconversion)
    endif ()
endif ()
