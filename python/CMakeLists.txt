find_package(Python 3.11 REQUIRED COMPONENTS Interpreter Development.Module Development.SABIModule)

python_add_library(_zelda64 MODULE WITH_SOABI USE_SABI 3.11
        src/module.c)

target_link_libraries(_zelda64
        PRIVATE zelda64::zelda64_static)

install(TARGETS _zelda64 DESTINATION zelda64)
