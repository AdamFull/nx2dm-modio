
option(NX_MODULE_MODIO "Build the mod.io user-generated content module" OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/NxModio.cmake")

if (NOT NX_MODULE_MODIO)
    return()
endif ()

nx_add_modio()

nx_module(modio
        DISPLAY "mod.io user-generated content"
        SOURCES modio_module.cpp modio_service.cpp modio_scripting.cpp)
target_link_libraries(nx_module_modio PUBLIC modioStatic)

if (NX_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
