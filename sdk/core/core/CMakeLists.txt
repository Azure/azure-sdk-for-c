# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_core LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (az_core src/az_json_read.c src/az_net.c)

if(MSVC)
  target_compile_options(az_core PRIVATE /W4 /WX)
else()
  target_compile_options(az_core PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

target_include_directories (az_core PUBLIC inc)

# make sure that users can consume the project as a library.
add_library (az::core ALIAS az_core)

add_executable (az_core_test test/main.c)

target_link_libraries(az_core_test PRIVATE az_core)

if (UNIX)
  target_link_libraries(az_core_test PRIVATE m)
endif()

add_test(NAME az_core_test COMMAND az_core_test)
