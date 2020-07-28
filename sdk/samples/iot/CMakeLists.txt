# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

if (TRANSPORT_PAHO)

cmake_minimum_required (VERSION 3.10)

set(TARGET_NAME "az_iot_samples_common")

project (${TARGET_NAME} LANGUAGES C)

set(CMAKE_C_STANDARD 99)

find_package(eclipse-paho-mqtt-c CONFIG)
if(NOT eclipse-paho-mqtt-c_FOUND)
  find_package(eclipse-paho-mqtt-c REQUIRED)
endif()

find_package(OpenSSL CONFIG)
if(NOT OpenSSL_FOUND)
  find_package(OpenSSL REQUIRED)
endif()

# Azure IoT Samples Library
add_library (${TARGET_NAME}
  ${CMAKE_CURRENT_LIST_DIR}/src/sample_sas_utility.c
)

target_include_directories (${TARGET_NAME}
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/inc
)

# Workaround for linker warning LNK4098: defaultlib 'LIBCMTD' conflicts with use of other libs
if (MSVC)
    set_target_properties(${TARGET_NAME}
        PROPERTIES LINK_FLAGS
        "/VERBOSE:LIB"
        #"/NODEFAULTLIB:libcmtd.lib"
    )
endif()

# Internal deps
target_link_libraries(${TARGET_NAME}
  PUBLIC
    az::iot::hub
    az::iot::provisioning
)

# External deps
target_link_libraries(${TARGET_NAME}
  PUBLIC
    eclipse-paho-mqtt-c::paho-mqtt3cs-static
    OpenSSL::SSL
    OpenSSL::Crypto
)

add_library (az::iot::samples::common ALIAS ${TARGET_NAME})

add_subdirectory(hub)
add_subdirectory(provisioning)

endif() # TRANSPORT_PAHO

