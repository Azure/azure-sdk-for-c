# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_iot_hub_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddTestCMocka)

# Generate map file
if(MSVC)
    add_link_options(/MAP)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_link_options(-Wl,-map,az_iot_hub_test.map)
else()
    add_link_options(-Xlinker -Map=az_iot_hub_test.map)
endif()

add_cmocka_test(az_iot_hub_test SOURCES
                main.c
                test_az_iot_hub_client_sas.c
                test_az_iot_hub_client_telemetry.c
                test_az_iot_hub_client_c2d.c
                test_az_iot_hub_client.c
                test_az_iot_hub_client_twin.c
                test_az_iot_hub_client_methods.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_TARGETS
                    az_iot_common
                    az_iot_hub
                    az_core
                    ${PAL}
                )
