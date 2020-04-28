# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (BUILD_PAHO_TRANSPORT)

cmake_minimum_required (VERSION 3.10)

project (az_iot_hub_samples LANGUAGES C)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/c2d)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/telemetry)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/twin)

endif() #BUILD_PAHO_TRANSPORT
