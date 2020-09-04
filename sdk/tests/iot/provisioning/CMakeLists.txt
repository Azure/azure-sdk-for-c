# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_iot_provisioning_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddTestCMocka)

create_map_file(az_iot_provisioning_test.map)

add_cmocka_test(az_iot_provisioning_test SOURCES
                main.c
                test_az_iot_provisioning_client.c
                test_az_iot_provisioning_client_sas.c
                test_az_iot_provisioning_client_parser.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_TARGETS
                    az_iot_common
                    az_iot_provisioning
                    az_core
                    ${PAL}
                )
