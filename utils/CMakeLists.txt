add_executable(decompress
        decompress.c
        rom.c
        rom.h)

target_link_libraries(decompress
        PRIVATE zelda64::zelda64_static)

zelda64_enable_warnings(decompress)
