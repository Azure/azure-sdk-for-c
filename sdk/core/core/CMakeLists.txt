# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)
set(TARGET_NAME "az_core")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/version.txt)
    file (STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/version.txt VERSION_STRING)
    message(STATUS "Read version from version.txt: " ${VERSION_STRING})
    string(REGEX MATCHALL "([0-9]+).([0-9]+).([0.9]+)(.*)" PRE_RELEASE ${VERSION_STRING})

    set(AZ_VERSION_MAJOR ${CMAKE_MATCH_1} )
    set(AZ_VERSION_MINOR ${CMAKE_MATCH_2} )
    set(AZ_VERSION_PATCH ${CMAKE_MATCH_3} )
    if(CMAKE_MATCH_4)
        string(SUBSTRING ${CMAKE_MATCH_4} 1 -1 PRERELEASE_SUBSTRING )
        set(AZ_VERSION_PRERELEASE ${PRERELEASE_SUBSTRING})
    endif()
    message(STATUS "AZ_VERSION_MAJOR " ${AZ_VERSION_MAJOR})
    message(STATUS "AZ_VERSION_MINOR " ${AZ_VERSION_MINOR})
    message(STATUS "AZ_VERSION_PATCH " ${AZ_VERSION_PATCH})
    message(STATUS "AZ_VERSION_PRERELEASE " ${AZ_VERSION_PRERELEASE})
else()
  message(FATAL_ERROR "Unable to read version file")
endif()

project (${TARGET_NAME} LANGUAGES C VERSION ${AZ_VERSION_MAJOR}.${AZ_VERSION_MINOR}.${AZ_VERSION_PATCH})
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
  src/az_json_parser.c
  src/az_json_pointer.c
  src/az_json_string.c
  src/az_json_token.c
  src/az_log.c
  src/az_precondition.c
  src/az_span.c
  src/az_version.c
  )

target_include_directories (${TARGET_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/${TARGET_NAME}>)
# include internal headers
target_include_directories(${TARGET_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/internal> $<INSTALL_INTERFACE:include/az_core_internal>)

# include test internal headers
target_include_directories(${TARGET_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test/inc>)

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

target_compile_definitions(${TARGET_NAME} PRIVATE "AZ_VERSION_MAJOR=${AZ_VERSION_MAJOR}")
target_compile_definitions(${TARGET_NAME} PRIVATE "AZ_VERSION_MINOR=${AZ_VERSION_MINOR}")
target_compile_definitions(${TARGET_NAME} PRIVATE "AZ_VERSION_PATCH=${AZ_VERSION_PATCH}")
target_compile_definitions(${TARGET_NAME} PRIVATE "AZ_VERSION_PRERELEASE=\"${AZ_VERSION_PRERELEASE}\"")

include(CreateCodeCoverageTargets)
