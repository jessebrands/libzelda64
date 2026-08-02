if (ZELDA64_USE_LIBC)
    add_executable(test_io test_io.c)
    target_link_libraries(test_io PRIVATE zelda64::zelda64_static)
    target_include_directories(test_io PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../lib")

    if (MSVC)
        target_compile_options(test_io PRIVATE /W4)
    else ()
        target_compile_options(test_io
                PRIVATE -Wall -Wextra
                PRIVATE -Wconversion
                PRIVATE -Wstrict-prototypes
                PRIVATE -Wold-style-definition)
    endif ()

    add_test(
            NAME Library.IO
            COMMAND test_io
    )
endif ()
