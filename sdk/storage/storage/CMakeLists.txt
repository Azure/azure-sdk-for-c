# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_storage LANGUAGE C)

set(CMAKE_C_STANDARD 99)

add_library (az_storage src/az_storage.c)
