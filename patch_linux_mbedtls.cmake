# Patches the fetched mod.io SDK's Linux platform CMake so its bundled
# mbedTLS is only built when nothing else in this configure has already
# defined an `mbedtls` target. See NxModio.cmake for why: nx2d's own
# third_party/mbedtls (curl's TLS backend on Linux, and GNS's when selected)
# is added first, and CMake refuses two add_subdirectory()s that both
# define a target of the same name - the exact class of failure this
# guards against ("Attempt to add link library 'mbedx509' to target
# 'mbedtls' which is not built in this directory").
#
# Runs with the fetched source tree as the working directory (FetchContent's
# PATCH_COMMAND convention), so paths below are relative to its root.

set(_file "platform/linux/CMakeLists.txt")
if(EXISTS "${_file}")
    file(READ "${_file}" _content)
    string(FIND "${_content}" "if(NOT TARGET mbedtls)" _already_patched)
    if(_already_patched EQUAL -1)
        string(REPLACE
                "execute_process(COMMAND make generated_files WORKING_DIRECTORY \${MODIO_ROOT_DIR}/ext/mbedtls)"
                "if(NOT TARGET mbedtls)\n\texecute_process(COMMAND make generated_files WORKING_DIRECTORY \${MODIO_ROOT_DIR}/ext/mbedtls)\n\tendif()"
                _content "${_content}")
        string(REPLACE
                "add_subdirectory(\${MODIO_ROOT_DIR}/ext/mbedtls mbedtls EXCLUDE_FROM_ALL)"
                "if(NOT TARGET mbedtls)\n\tadd_subdirectory(\${MODIO_ROOT_DIR}/ext/mbedtls mbedtls EXCLUDE_FROM_ALL)\n\tendif()"
                _content "${_content}")
        file(WRITE "${_file}" "${_content}")
    endif()
endif()
