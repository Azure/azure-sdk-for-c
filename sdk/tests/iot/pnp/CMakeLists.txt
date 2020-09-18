# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_iot_pnp_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddTestCMocka)

add_cmocka_test(az_iot_pnp_test SOURCES
                main.c
                test_az_iot_pnp_client_telemetry.c
                test_az_iot_pnp_client.c
                test_az_iot_pnp_client_twin.c
                test_az_iot_pnp_client_commands.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_TARGETS
                    az_iot_common
                    az_iot_hub
                    az_iot_pnp
                    az_core
                    ${PAL}
                )
