# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_nohttp LANGUAGES C)

add_library (
  az_nohttp STATIC
  src/az_nohttp.c
  )

target_link_libraries(az_nohttp PUBLIC az_core)

# make sure that users can consume the project as a library.
add_library (az::nohttp ALIAS az_nohttp)
