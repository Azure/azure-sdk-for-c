# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.12)

project (az_core LANGUAGES C)

set(CMAKE_C_STANDARD 99)
find_package(CURL CONFIG)
if(NOT CURL_FOUND)
  find_package(CURL REQUIRED)
endif()

add_library (
  az_core
  src/az_assumptions.c
  src/az_base64.c
  src/az_curl_adapter.c
  src/az_curl_slist.c
  src/az_http_header.c
  src/az_http_pipeline.c
  src/az_http_policy.c
  src/az_http_query.c
  src/az_http_request.c
  src/az_http_request_builder.c
  src/az_http_response_parser.c
  src/az_json_builder.c
  src/az_json_get.c
  src/az_json_parser.c
  src/az_json_pointer.c
  src/az_json_string.c
  src/az_json_token.c
  src/az_mut_span.c
  src/az_pair.c
  src/az_span.c
  src/az_span_builder.c
  src/az_span_malloc.c
  src/az_span_span.c
  src/az_span_reader.c
  src/az_span_writer.c
  src/az_str.c
  src/az_uri.c
  src/az_url.c
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

target_include_directories (az_core PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include>)

# create interface for internal headers
add_library(az_core_internal INTERFACE)
target_include_directories(az_core_internal INTERFACE internal)

target_link_libraries(az_core PRIVATE az_core_internal)

# make sure that users can consume the project as a library.
add_library (az::core ALIAS az_core)

add_executable (
    az_core_test 
    test/main.c 
    test/test_json_builder.c
    test/test_json_get_by_pointer.c
    test/test_json_pointer.c
    test/test_json_string.c  
    test/test_url_parse.c
    )

target_link_libraries(az_core_test PRIVATE az_core az_core_internal)
target_include_directories (az_core_test PRIVATE src)

target_link_libraries(az_core PRIVATE CURL::libcurl)

if (MOCK_CURL)
  MESSAGE("Will mock all calls to http client with static responses")
  add_definitions(-DMOCK_CURL="MOCK")
endif()

if (UNIX)
  target_link_libraries(az_core_test PRIVATE m)
endif()

add_test(NAME az_core_test COMMAND az_core_test)
