# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if(NOT WIN32)

cmake_minimum_required(VERSION 3.12)

project(az_posix LANGUAGES C)
set(CMAKE_C_STANDARD 99)

add_library(az_posix STATIC)

target_link_libraries(az_posix PRIVATE az_core)

target_sources(
    az_posix
        PRIVATE
            src/az_posix.c
    )

target_include_directories(
    az_posix
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    )

endif()
