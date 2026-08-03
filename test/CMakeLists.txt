if (ZELDA64_USE_LIBC)
    add_executable(test_io test_io.c)
    target_link_libraries(test_io PRIVATE zelda64::zelda64_static)
    zelda64_enable_warnings(test_io)

    target_include_directories(test_io PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../lib")

    add_test(
            NAME Library.IO
            COMMAND test_io
    )
endif ()
