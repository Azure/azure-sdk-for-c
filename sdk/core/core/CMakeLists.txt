# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_core LANGUAGES C)

set(CMAKE_C_STANDARD 99)

target_include_directories (az_core PUBLIC inc)

add_library (az_core src/az_core.c)

# make sure that users can consume the project as a library.
add_library (az::core ALIAS az_core)

