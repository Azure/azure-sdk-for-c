# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_core_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_executable (az_keyvault_test main.c)
target_link_libraries(az_keyvault_test PUBLIC az_keyvault az_core)
target_include_directories (az_keyvault_test PRIVATE .)

add_test(NAME az_keyvault_test COMMAND az_keyvault_test)
