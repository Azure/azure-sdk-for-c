# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_core LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (az_core src/az_core.c src/az_json_read.c)

target_include_directories (az_core PUBLIC inc)

# make sure that users can consume the project as a library.
add_library (az::core ALIAS az_core)

add_executable (az_core_test test/main.c)

target_include_directories (az_core_test PRIVATE inc)
target_link_libraries(az_core_test PRIVATE az_core)

add_test(NAME az_core_test COMMAND ${EXECUTABLE_OUTPUT_PATH}/az_core_test)
