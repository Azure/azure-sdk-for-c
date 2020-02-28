project(core-utest C)

include(AddTestCMocka)

add_cmocka_test(test_json SOURCES
                test_az_json.c
                test_json_value.c
                test_json_string.c
                test_json_pointer.c
                test_json_parser.c
                test_json_get_by_pointer.c
                test_json_builder.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                PRIVATE_ACCESS ON)

add_cmocka_test(test_az_span SOURCES
                test_az_span.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS})

