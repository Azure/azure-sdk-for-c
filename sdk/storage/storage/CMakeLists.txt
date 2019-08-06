# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_storage LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (az_storage src/az_storage.c)

# make sure that users can consume the project as a library.
add_library (az::storage ALIAS az_storage)
