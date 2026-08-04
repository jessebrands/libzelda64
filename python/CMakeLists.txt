find_package(Python 3.9 REQUIRED COMPONENTS
        Interpreter
        Development.Module
        Development.SABIModule)

set(ZELDA64_MODULE_SOURCES
        module.c)

set(ZELDA64_MODULE_FILES
        zelda64.pyi
        zelda64_cli.py)

python_add_library(zelda64_module MODULE WITH_SOABI USE_SABI 3.9
        ${ZELDA64_MODULE_SOURCES})

target_link_libraries(zelda64_module
        PRIVATE zelda64::zelda64_static)

set_target_properties(zelda64_module PROPERTIES
        OUTPUT_NAME zelda64)

zelda64_enable_warnings(zelda64_module)

install(TARGETS zelda64_module DESTINATION .)
install(FILES ${ZELDA64_MODULE_FILES} DESTINATION .)

