set(ZELDA64_TEST_HEADERS
        arena.h
        check.h)

if (ZELDA64_USE_LIBC)
    add_executable(test_io test_io.c ${ZELDA64_TEST_HEADERS})
    target_link_libraries(test_io PRIVATE zelda64::zelda64_static)
    zelda64_enable_warnings(test_io)

    target_include_directories(test_io PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")

    add_test(
            NAME Library.IO
            COMMAND test_io
    )
endif ()

add_executable(test_dma test_dma.c ${ZELDA64_TEST_HEADERS})
target_link_libraries(test_dma PRIVATE zelda64::zelda64_static)
zelda64_enable_warnings(test_dma)

add_test(NAME Library.DMA COMMAND test_dma)
