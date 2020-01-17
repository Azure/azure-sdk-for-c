# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_identity LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (
  az_identity
  src/az_identity_client_secret_credential.c
  )

target_include_directories (az_identity PUBLIC inc)

target_link_libraries(az_identity PUBLIC az_core)
target_link_libraries(az_identity PRIVATE az_core_internal)

# make sure that users can consume the project as a library.
add_library (az::identity ALIAS az_identity)

# add_executable (az_identity_test test/main.c)
# target_link_libraries(az_identity_test PRIVATE az_identity)

# curl easy perform POC
add_executable (az_identity_poc test/curl_easy_perform_poc.c)
target_link_libraries(az_identity_poc PRIVATE az_identity az_core_internal)

if (UNIX)
  target_link_libraries(az_identity PRIVATE m)
endif()
