# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

set(CMAKE_C_STANDARD 99)

add_executable (
    az_iot_tests
    main.c
)

target_link_libraries(az_iot_tests PRIVATE az_iot az_core_internal)
target_include_directories (az_iot_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (UNIX)
  target_link_libraries(az_iot_tests PRIVATE m)
endif()

add_test(NAME az_iot_tests COMMAND az_iot_tests)
