project(iot-unit-test C)

include(AddTestCMocka)

add_cmocka_test(az_iot_test SOURCES
                main.c
                az_iot_sas_token_tests.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_TARGETS
                    az_core
                    az_iot
                )
