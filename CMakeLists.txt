# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)
project(az LANGUAGES C)
enable_testing ()
add_subdirectory(sdk/core/core)
add_subdirectory(sdk/storage/ustorageclient)
