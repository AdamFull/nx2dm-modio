
option(NX_MODULE_MODIO "Build the mod.io user-generated content module" OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/NxModio.cmake")

if (NOT NX_MODULE_MODIO)
    return()
endif ()

nx_add_modio()

set(NX_MODIO_SOURCES
        modio_module.cpp modio_service.cpp modio_scripting.cpp
        modio_config.cpp
        modio_user.cpp modio_authoring.cpp modio_monetization.cpp
        modio_advanced.cpp modio_collections.cpp modio_server.cpp)
if (ANDROID)
    list(APPEND NX_MODIO_SOURCES modio_android.cpp)
endif ()

nx_module(modio
        DISPLAY "mod.io user-generated content"
        SOURCES ${NX_MODIO_SOURCES})
target_link_libraries(nx_module_modio PUBLIC modioStatic)

if (ANDROID)
    # ModioAndroid.h is compiled into modioStatic but the SDK exports no
    # include path for its platform layer, so name it ourselves. SDL is where
    # the JNI environment and activity come from.
    target_include_directories(nx_module_modio PRIVATE
            "${NX_MODIO_SDK_ROOT}/platform/android/android")
    target_link_libraries(nx_module_modio PRIVATE nx::sdl)
endif ()

if (NX_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
