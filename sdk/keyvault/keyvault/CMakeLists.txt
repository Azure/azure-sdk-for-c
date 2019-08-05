# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_keyvault LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (az_keyvault src/az_keyvault.c)

# make sure that users can consume `az_keyvault` as a library.
add_library (az::keyvault ALIAS az_keyvault)