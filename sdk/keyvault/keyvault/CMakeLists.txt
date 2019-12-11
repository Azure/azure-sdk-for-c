# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_keyvault LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (
  az_keyvault
  src/az_keyvault_client.c
  )

if(MSVC)
  target_compile_options(az_keyvault PRIVATE /Wall /WX /wd5031 /wd4668 /wd4820 /wd4255 /analyze)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
  target_compile_options(az_keyvault PRIVATE -Wall -Wextra -pedantic -Werror -Wdocumentation  -Wdocumentation-unknown-command)
else()
  target_compile_options(az_keyvault PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

target_include_directories (az_keyvault PUBLIC inc)

target_link_libraries(az_keyvault PUBLIC az_core)

# make sure that users can consume the project as a library.
add_library (az::keyvault ALIAS az_keyvault)

add_executable (az_keyvault_test test/main.c)
target_link_libraries(az_keyvault_test PRIVATE az_keyvault)

# make key vault POC
add_executable (az_keyvault_poc test/client_key_poc.c)
target_link_libraries(az_keyvault_poc PRIVATE az_keyvault)

if (UNIX)
  target_link_libraries(az_keyvault PRIVATE m)
endif()

add_test(NAME az_keyvault_test COMMAND az_keyvault_test)
