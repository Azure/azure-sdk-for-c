project(core-utest C)

include(AddTestCMocka)

add_cmocka_test(az_keyvault_test SOURCES
                main.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_TARGETS
                    az_core
                    az_keyvault
                )
