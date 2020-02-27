project(core-utest C)

include(CMakeLists_cmoka_add_test_function.txt)

add_cmocka_test(test_json
                SOURCES test_az_json.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS})

add_cmocka_test(test_az_span
                SOURCES test_az_span.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS})

