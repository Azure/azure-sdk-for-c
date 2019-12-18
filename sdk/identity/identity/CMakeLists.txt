# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_identity LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_library (
  az_identity
  src/az_identity_client_secret_credential.c
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

target_include_directories (az_identity PUBLIC inc)

target_link_libraries(az_identity PUBLIC az_core)

# make sure that users can consume the project as a library.
add_library (az::identity ALIAS az_identity)

# add_executable (az_identity_test test/main.c)
# target_link_libraries(az_identity_test PRIVATE az_identity)

# curl easy perform POC
add_executable (az_identity_poc test/curl_easy_perform_poc.c)
target_link_libraries(az_identity_poc PRIVATE az_identity)

if (UNIX)
  target_link_libraries(az_identity PRIVATE m)
endif()
