# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)
set(TARGET_NAME "az_core")

# Read in az_version.h
#   The build will fail with an error if the file is not found or improperly formatted
set(AZ_SDK_VERSION_H_FILE "${CMAKE_CURRENT_SOURCE_DIR}/inc/az_version.h")

if(EXISTS ${AZ_SDK_VERSION_H_FILE})
    file (STRINGS ${AZ_SDK_VERSION_H_FILE} VERSION_H_CONTENT)
    message(STATUS "Read version from ${AZ_SDK_VERSION_H_FILE}")

    #Find and read the AZ_SDK_VERSION_STRING define
    string(REGEX MATCHALL "#define AZ_SDK_VERSION_STRING \"([0-9]+)\.([0-9]+)\.([0-9]+)(\-[^\"\-]+)?\"" VERSION_PARTS ${VERSION_H_CONTENT})
    
    #Ensure we matched as expected.
    #   MAJOR.AZ_VERSION_MINOR.PATCH are required. 
    #   PRERELEASE is optional.
    if(NOT CMAKE_MATCH_1 AND NOT CMAKE_MATCH_2 AND NOT CMAKE_MATCH_3)
        message(FATAL_ERROR "Unexpected version format in ${AZ_SDK_VERSION_H_FILE}")
    endif()
    set(AZ_SDK_VERSION_MAJOR ${CMAKE_MATCH_1} )
    set(AZ_SDK_VERSION_MINOR ${CMAKE_MATCH_2} )
    set(AZ_SDK_VERSION_PATCH ${CMAKE_MATCH_3} )
    if(CMAKE_MATCH_4)
        #Trim off the "-" from the start of pre-release portion
        # https://cmake.org/cmake/help/v3.10/command/string.html?highlight=substring#substring
        string(SUBSTRING ${CMAKE_MATCH_4} 1 -1 PRERELEASE_SUBSTRING )
        set(AZ_SDK_VERSION_PRERELEASE ${PRERELEASE_SUBSTRING})
    endif()
    message(STATUS "AZ_SDK_VERSION_MAJOR " ${AZ_SDK_VERSION_MAJOR})
    message(STATUS "AZ_SDK_VERSION_MAJOR " ${AZ_SDK_VERSION_MINOR})
    message(STATUS "AZ_SDK_VERSION_PATCH " ${AZ_SDK_VERSION_PATCH})
    message(STATUS "AZ_SDK_VERSION_PRERELEASE " ${AZ_SDK_VERSION_PRERELEASE})
else()
  message(FATAL_ERROR "Missing Version file ${AZ_SDK_VERSION_H_FILE}")
endif()

project (${TARGET_NAME} LANGUAGES C VERSION ${AZ_SDK_VERSION_MAJOR}.${AZ_SDK_VERSION_MINOR}.${AZ_SDK_VERSION_PATCH})
set(CMAKE_C_STANDARD 99)

include(CheckAndIncludeCodeCov)

add_library (
  ${TARGET_NAME}
  src/az_aad.c
  src/az_credential_client_secret.c
  src/az_credential_token.c
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
  src/az_spinlock.c)

target_include_directories (${TARGET_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/${TARGET_NAME}>)
# include internal headers
target_include_directories(${TARGET_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/internal> $<INSTALL_INTERFACE:include/az_core_internal>)

# include test internal headers
target_include_directories(${TARGET_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test/inc>)

# make sure that users can consume the project as a library.
add_library (az::core ALIAS ${TARGET_NAME})

include(CreateCodeCoverageTargets)
