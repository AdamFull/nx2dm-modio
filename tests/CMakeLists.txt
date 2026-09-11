
add_executable(nx_modio_tests
        test_modio_service.cpp
        test_modio_user.cpp
        test_modio_advanced.cpp
        test_modio_collections.cpp
        test_modio_smoke.cpp
        test_modio_scripting.cpp
        test_modio_config.cpp
        ../modio_module.cpp
        ../modio_service.cpp
        ../modio_scripting.cpp
        ../modio_config.cpp
        ../modio_user.cpp
        ../modio_authoring.cpp
        ../modio_monetization.cpp
        ../modio_advanced.cpp
        ../modio_collections.cpp
        ../modio_server.cpp)

nx_module_registry(nx_modio_tests)

target_include_directories(nx_modio_tests PRIVATE
        "${NX_ENGINE_ROOT}/engine" "${NX_ENGINE_ROOT}/modules")

target_link_libraries(nx_modio_tests
        PRIVATE nx::test nx::foundation nx::engine modioStatic nx::compile_options)

set_target_properties(nx_modio_tests PROPERTIES FOLDER "tests")
nxtest_discover_tests(nx_modio_tests LABELS modio)
