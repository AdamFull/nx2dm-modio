
add_executable(nx_modio_tests
        test_modio_service.cpp
        ../modio_service.cpp)

target_include_directories(nx_modio_tests PRIVATE
        "${NX_ENGINE_ROOT}/engine" "${NX_ENGINE_ROOT}/modules")

target_link_libraries(nx_modio_tests
        PRIVATE nx::test nx::foundation modioStatic nx::compile_options)

set_target_properties(nx_modio_tests PROPERTIES FOLDER "tests")
nxtest_discover_tests(nx_modio_tests LABELS modio)
