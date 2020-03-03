project(core-utest C)

include(AddTestCMocka)

add_cmocka_test(az_core_test SOURCES
                main.c

                test_az_json.c
                test_json_value.c
                test_json_string.c
                test_json_pointer.c
                test_json_parser.c
                test_json_get_by_pointer.c
                test_json_builder.c

                test_az_span.c
                test_span.c
                test_span_replace.c

                test_az_context.c

                test_log.c

                test_http_request.c
                test_http_response_read_and_parse.c

                test_url_encode.c

                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                # grant access to Private functions to test az_json_private
                PRIVATE_ACCESS ON)
