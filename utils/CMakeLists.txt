add_executable(decompress
        decompress.c)

target_link_libraries(decompress
        PRIVATE zelda64::zelda64_static)

zelda64_enable_warnings(decompress)
