# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_keyvault_keys_samples LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# make keys client example
add_executable (keys_client_example src/keys_client_example.c)
target_link_libraries(keys_client_example PRIVATE az_keyvault az_core az_core_internal az_curl)

if (UNIX)
  target_link_libraries(keys_client_example PRIVATE m)
endif()

# TODO, Add tests for samples
# add_test(NAME az_keyvault_test COMMAND az_keyvault_test)
