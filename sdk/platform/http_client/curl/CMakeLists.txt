# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (BUILD_CURL_TRANSPORT)

cmake_minimum_required (VERSION 3.12)

project (az_curl LANGUAGES C)

set(CMAKE_C_STANDARD 99)
find_package(CURL CONFIG)
if(NOT CURL_FOUND)
  find_package(CURL REQUIRED)
endif()

add_library (
  az_curl STATIC
  src/az_curl.c
  )

if(MSVC)
  if(WARNINGS_AS_ERRORS)
    set(WARNINGS_AS_ERRORS_FLAG "/WX")
  endif()

  target_compile_options(az_curl PRIVATE /Wall ${WARNINGS_AS_ERRORS_FLAG} /wd5031 /wd4668 /wd4820 /wd4255 /analyze)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
  if(WARNINGS_AS_ERRORS)
    set(WARNINGS_AS_ERRORS_FLAG "-Werror")
  endif()

  target_compile_options(az_curl PRIVATE -Xclang -Wall -Wextra -pedantic  ${WARNINGS_AS_ERRORS_FLAG} -Wdocumentation -Wdocumentation-unknown-command -fcomment-block-commands=retval -Wcast-qual)
else()
  if(WARNINGS_AS_ERRORS)
    set(WARNINGS_AS_ERRORS_FLAG "-Werror")
  endif()

  target_compile_options(az_curl PRIVATE -Wall -Wextra -pedantic  ${WARNINGS_AS_ERRORS_FLAG})
endif()

target_link_libraries(az_curl PRIVATE az_core az_core_internal)

# make sure that users can consume the project as a library.
add_library (az::curl ALIAS az_curl)


target_link_libraries(az_curl PRIVATE CURL::libcurl)

endif()
