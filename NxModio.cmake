set(NX_MODIO_REPOSITORY "https://github.com/modio/modio-sdk.git"
    CACHE STRING "Where to fetch the mod.io C++ SDK from")

# modio-sdk has no tagged releases; this is main's HEAD as of the integration.
set(NX_MODIO_TAG "3c7c680a82b91e9df4d5f675284f6d1a2fe56109"
    CACHE STRING "modio-sdk commit to build against")

set(NX_MODIO_SOURCE_DIR "" CACHE PATH
    "An existing modio-sdk checkout (with submodules). Empty fetches NX_MODIO_TAG.")

function(_nx_modio_platform_string out_var)
    if (ANDROID)
        set(${out_var} "ANDROID" PARENT_SCOPE)
    elseif (IOS)
        set(${out_var} "IOS" PARENT_SCOPE)
    elseif (APPLE)
        set(${out_var} "MACOS" PARENT_SCOPE)
    elseif (WIN32)
        set(${out_var} "WIN" PARENT_SCOPE)
    else ()
        set(${out_var} "LINUX" PARENT_SCOPE)
    endif ()
endfunction()

# Vendors the mod.io SDK and defines the modioStatic target this module links
# against. The SDK's own top-level CMakeLists.txt requires MODIO_PLATFORM to
# already be set before it is processed (it gates on this ahead of its own
# project() call), and pulls in its dozen-odd submodule dependencies (asio,
# mbedtls, nlohmann/json, fmt, ...) itself - there is nothing left to wire up
# beyond selecting the platform and making the source available.
function(nx_add_modio)
    if (TARGET modioStatic)
        return()
    endif ()

    _nx_modio_platform_string(_modio_platform)
    set(MODIO_PLATFORM "${_modio_platform}" CACHE STRING "mod.io SDK target platform" FORCE)

    if (NX_MODIO_SOURCE_DIR)
        if (NOT EXISTS "${NX_MODIO_SOURCE_DIR}/modio/modio/ModioSDK.h")
            message(FATAL_ERROR
                "NX_MODIO_SOURCE_DIR='${NX_MODIO_SOURCE_DIR}' holds no modio-sdk checkout: "
                "expected modio/modio/ModioSDK.h there.")
        endif ()
        message(STATUS "nx2d: mod.io SDK from ${NX_MODIO_SOURCE_DIR}")
        add_subdirectory("${NX_MODIO_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_deps/modio-build")
    else ()
        include(FetchContent)
        find_package(Git 2.25 REQUIRED)
        message(STATUS "nx2d: fetching mod.io SDK ${NX_MODIO_TAG} (platform ${MODIO_PLATFORM})")
        # On Linux, modio's own platform CMake unconditionally builds its
        # bundled ext/mbedtls, colliding by target name with nx2d's own
        # third_party/mbedtls (curl's TLS backend there, and GNS's when
        # selected) whenever both are configured in the same run - see
        # patch_linux_mbedtls.cmake.
        FetchContent_Declare(modio_sdk
                GIT_REPOSITORY "${NX_MODIO_REPOSITORY}"
                GIT_TAG "${NX_MODIO_TAG}"
                GIT_SUBMODULES_RECURSE TRUE
                GIT_PROGRESS TRUE
                PATCH_COMMAND "${CMAKE_COMMAND}" -P
                "${CMAKE_CURRENT_LIST_DIR}/patch_linux_mbedtls.cmake")
        FetchContent_MakeAvailable(modio_sdk)
    endif ()

    if (NOT TARGET modioStatic)
        message(FATAL_ERROR "nx2d: mod.io SDK fetched but defines no 'modioStatic' target")
    endif ()
endfunction()
