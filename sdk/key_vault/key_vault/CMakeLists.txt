# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_key_vault LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (
  az_key_vault
  src/az_key_vault_client.c
  )

if(MSVC)
  target_compile_options(az_key_vault PRIVATE /Wall /WX /wd5031 /wd4668 /wd4820 /wd4255 /analyze)
else()
  target_compile_options(az_key_vault PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

target_include_directories (az_key_vault PUBLIC inc)

target_link_libraries(az_key_vault PUBLIC az_core)

# make sure that users can consume the project as a library.
add_library (az::key_vault ALIAS az_key_vault)

add_executable (az_key_vault_test test/main.c)
target_link_libraries(az_key_vault_test PRIVATE az_key_vault)

# make key vault POC
add_executable (az_key_vault_POC test/client_key_POC.c)
target_link_libraries(az_key_vault_POC PRIVATE az_key_vault)

if (UNIX)
  target_link_libraries(az_key_vault PRIVATE m)
endif()

add_test(NAME az_key_vault_test COMMAND az_key_vault_test)
