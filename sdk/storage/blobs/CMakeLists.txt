# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_storage_blobs LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (
  az_storage_blobs
  src/az_storage_blobs_blob_client.c
  )

if(MSVC)
  if(WARNINGS_AS_ERRORS)
    set(WARNINGS_AS_ERRORS_FLAG "/WX")
  endif()

  target_compile_options(az_core PRIVATE /Wall ${WARNINGS_AS_ERRORS_FLAG} /wd5031 /wd4668 /wd4820 /wd4255 /analyze)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
  if(WARNINGS_AS_ERRORS)
    set(WARNINGS_AS_ERRORS_FLAG "-Werror")
  endif()

  target_compile_options(az_core PRIVATE -Xclang -Wall -Wextra -pedantic  ${WARNINGS_AS_ERRORS_FLAG} -Wdocumentation -Wdocumentation-unknown-command -Wcast-qual)
else()
  if(WARNINGS_AS_ERRORS)
    set(WARNINGS_AS_ERRORS_FLAG "-Werror")
  endif()

  target_compile_options(az_core PRIVATE -Wall -Wextra -pedantic  ${WARNINGS_AS_ERRORS_FLAG})
endif()

target_include_directories (az_storage_blobs PUBLIC inc)

target_link_libraries(az_storage_blobs PUBLIC az_core az_identity az_core_internal)
target_link_libraries(az_storage_blobs PRIVATE az_core_internal)

# make sure that users can consume the project as a library.
add_library (az::storage::blobs ALIAS az_storage_blobs)

add_executable (az_storage_blobs_test test/main.c)
target_link_libraries(az_storage_blobs_test PRIVATE az_storage_blobs az_core_internal)

# make Storage POC
add_executable (az_storage_blobs_poc test/storage_blobs_poc.c)
target_link_libraries(az_storage_blobs_poc PRIVATE az_storage_blobs)

if (UNIX)
  target_link_libraries(az_storage_blobs PRIVATE m)
endif()

add_test(NAME az_storage_blobs_test COMMAND az_storage_blobs_test)
