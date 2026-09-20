# Patches the fetched mod.io SDK's per-platform CMake so its bundled mbedTLS
# is only built when nothing else in this configure has already defined an
# `mbedtls` target. See NxModio.cmake for why: nx2d's own third_party/mbedtls
# (curl's TLS backend on Linux and Android, and GNS's when selected) is added
# first, and CMake refuses two add_subdirectory()s that both define a target of
# the same name - the exact class of failure this guards against ("Attempt to
# add link library 'mbedx509' to target 'mbedtls' which is not built in this
# directory").
#
# Both the Linux and the Android platform scripts build it; the other platforms
# link a system or vendor TLS stack instead and need no patch. Whichever is
# skipped, the target_compile_options(mbedcrypto ...) that follows is left
# alone on purpose - it then applies to nx2d's own mbedcrypto, which wants the
# same warning suppressions.
#
# Runs with the fetched source tree as the working directory (FetchContent's
# PATCH_COMMAND convention), so paths below are relative to its root.

foreach (_file "platform/linux/CMakeLists.txt" "platform/android/CMakeLists.txt")
    if (NOT EXISTS "${_file}")
        continue()
    endif ()

    file(READ "${_file}" _content)
    string(FIND "${_content}" "if(NOT TARGET mbedtls)" _already_patched)
    if (NOT _already_patched EQUAL -1)
        continue()
    endif ()

    # Android's spelling of this line names the NDK's host prebuilt directory,
    # which differs per build host, so match the line rather than assume it.
    string(REGEX REPLACE
            "(execute_process\\(COMMAND [^\n]*make generated_files WORKING_DIRECTORY \\\${MODIO_ROOT_DIR}/ext/mbedtls\\))"
            "if(NOT TARGET mbedtls)\n\t\\1\n\tendif()"
            _content "${_content}")
    string(REPLACE
            "add_subdirectory(\${MODIO_ROOT_DIR}/ext/mbedtls mbedtls EXCLUDE_FROM_ALL)"
            "if(NOT TARGET mbedtls)\n\tadd_subdirectory(\${MODIO_ROOT_DIR}/ext/mbedtls mbedtls EXCLUDE_FROM_ALL)\n\tendif()"
            _content "${_content}")
    file(WRITE "${_file}" "${_content}")
endforeach ()
