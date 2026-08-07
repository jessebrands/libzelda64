find_package(yaz0 REQUIRED)

add_executable(decompress
        decompress.c)

target_link_libraries(decompress
        PRIVATE zelda64::zelda64_static
        PRIVATE yaz0::yaz0_static)
