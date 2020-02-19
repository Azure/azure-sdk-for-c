# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_keyvault LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library(
    az_keyvault
    src/az_keyvault_client.c
    )

target_include_directories(az_keyvault PUBLIC inc)
target_include_directories(az_keyvault PRIVATE src)

target_link_libraries(az_keyvault PUBLIC az_core)
target_link_libraries(az_keyvault PRIVATE az_core_internal)

# make sure that users can consume the project as a library.
add_library(az::keyvault ALIAS az_keyvault)

add_executable(
    az_keyvault_test
        test/main.c
    )

target_link_libraries(
    az_keyvault_test
        PRIVATE
            az_keyvault
            az_core_internal
    )

target_include_directories (
    az_keyvault_test
        PRIVATE
            src
    )

if (UNIX)
  target_link_libraries(az_keyvault PRIVATE m)
endif()

add_test(NAME az_keyvault_test COMMAND az_keyvault_test)
