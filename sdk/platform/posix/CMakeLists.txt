# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project(az_posix LANGUAGES C)
set(CMAKE_C_STANDARD 99)

add_library(az_posix STATIC)

target_link_libraries(az_posix PRIVATE az_core)

target_sources(az_posix PRIVATE src/az_posix.c)
