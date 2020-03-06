# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)
set(TARGET_NAME "az_core")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(CheckAndIncludeCodeCov)

add_library (
  ${TARGET_NAME}
  src/az_aad.c
  src/az_credential_client_secret.c
  src/az_context.c
  src/az_http_pipeline.c
  src/az_http_policy.c
  src/az_http_policy_logging.c
  src/az_http_policy_retry.c
  src/az_http_request.c
  src/az_http_response.c
  src/az_json_builder.c
  src/az_json_get.c
  src/az_json_parser.c
  src/az_json_pointer.c
  src/az_json_string.c
  src/az_json_token.c
  src/az_log.c
  src/az_span.c
  )

if(MSVC)
  if(WARNINGS_AS_ERRORS)
    set(WARNINGS_AS_ERRORS_FLAG "/WX")
  endif()

  target_compile_options(${TARGET_NAME} PRIVATE /Wall ${WARNINGS_AS_ERRORS_FLAG} /wd5031 /wd4668 /wd4820 /wd4255 /analyze)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
  if(WARNINGS_AS_ERRORS)
    set(WARNINGS_AS_ERRORS_FLAG "-Werror")
  endif()

  target_compile_options(${TARGET_NAME} PRIVATE -Xclang -Wall -Wextra -pedantic ${WARNINGS_AS_ERRORS_FLAG} -Wdocumentation -Wdocumentation-unknown-command -fcomment-block-commands=retval -Wcast-qual)
else()
  if(WARNINGS_AS_ERRORS)
    set(WARNINGS_AS_ERRORS_FLAG "-Werror")
  endif()

  target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -pedantic ${WARNINGS_AS_ERRORS_FLAG})
endif()

target_include_directories (${TARGET_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/${TARGET_NAME}>)
# include internal headers
target_include_directories(${TARGET_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/internal> $<INSTALL_INTERFACE:include/az_core_internal>)

set(AZ_PLATFORM_IMPL "NONE" CACHE STRING "Platform implementation to use")
set_property(CACHE AZ_PLATFORM_IMPL PROPERTY STRINGS NONE WIN32 POSIX USER)

set(AZ_USER_PLATFORM_IMPL_NAME "" CACHE STRING "Name of the implementation target")

if(AZ_PLATFORM_IMPL STREQUAL "USER")
  set(AZ_PLATFORM_IMPL_USER ON)
elseif(AZ_PLATFORM_IMPL STREQUAL "POSIX")
  set(AZ_PLATFORM_IMPL_POSIX ON)
elseif(AZ_PLATFORM_IMPL STREQUAL "WIN32")
  set(AZ_PLATFORM_IMPL_WIN32 ON)
else()
  set(AZ_PLATFORM_IMPL_NONE ON)
endif()

if(AZ_PLATFORM_IMPL_USER)
  target_link_libraries(${TARGET_NAME} PRIVATE ${AZ_USER_PLATFORM_IMPL_NAME})
elseif(AZ_PLATFORM_IMPL_POSIX)
  target_link_libraries(${TARGET_NAME} PRIVATE az_posix)
elseif(AZ_PLATFORM_IMPL_WIN32)
  target_link_libraries(${TARGET_NAME} PRIVATE az_win32)
elseif(AZ_PLATFORM_IMPL_NONE)
  target_link_libraries(${TARGET_NAME} PRIVATE az_noplatform)
endif()

if (BUILD_CURL_TRANSPORT)
  target_link_libraries(${TARGET_NAME} PRIVATE az_curl)
else()
  target_link_libraries(${TARGET_NAME} PRIVATE az_nohttp)
endif()

# make sure that users can consume the project as a library.
add_library (az::core ALIAS ${TARGET_NAME})

include(CreateCodeCoverageTargets)
