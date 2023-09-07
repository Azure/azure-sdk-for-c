# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project(az_platform LANGUAGES C)
set(CMAKE_C_STANDARD 99)

if(AZ_PLATFORM_IMPL STREQUAL "WIN32")
  # build windows platform
  add_library(az_win32 STATIC
    ${CMAKE_CURRENT_LIST_DIR}/az_win32.c
  )

  target_link_libraries(az_win32
    PRIVATE
      az_core
  )
elseif(AZ_PLATFORM_IMPL STREQUAL "POSIX")
  # build linux platform
  add_library(az_posix STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_posix.c
  )

  if (NOT APPLE)
    target_link_libraries(az_posix
      PRIVATE
        az_core
        -lrt
        -pthread
    )
  else()
    target_link_libraries(az_posix
      PRIVATE
        az_core
    )
  endif()
else()
  #noplatform
  add_library(az_noplatform STATIC
    ${CMAKE_CURRENT_LIST_DIR}/az_noplatform.c
  )

  target_link_libraries(az_noplatform
    PRIVATE
      az_core
  )
endif()

# HTTP Client
add_library (
  az_nohttp
    STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_nohttp.c
)

target_link_libraries(az_nohttp PRIVATE az_core)

# make sure that users can consume the project as a library.
add_library (az::nohttp ALIAS az_nohttp)

# Curl Platform
if (TRANSPORT_CURL)
  set(CURL_MIN_REQUIRED_VERSION 7.1) #Min curl version to support CURLOPT_HTTPHEADER option
  find_package(CURL ${CURL_MIN_REQUIRED_VERSION} CONFIG)
  if(NOT CURL_FOUND)
    find_package(CURL ${CURL_MIN_REQUIRED_VERSION} REQUIRED)
  endif()

  add_library (
    az_curl
      STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_curl.c
  )

  target_link_libraries(az_curl PRIVATE az_core)

  # make sure that users can consume the project as a library.
  add_library (az::curl ALIAS az_curl)

  target_link_libraries(az_curl PUBLIC CURL::libcurl)
  target_include_directories(az_curl INTERFACE ${CURL_INCLUDE_DIR})

endif()

# Mosquitto Platform

if (TRANSPORT_MOSQUITTO AND AZ_PLATFORM_IMPL STREQUAL "POSIX" AND (UNIX AND NOT APPLE))
  find_package(mosquitto  CONFIG)
  if(NOT mosquitto_FOUND)
    find_package(mosquitto REQUIRED)
  endif()

  find_package(OpenSSL CONFIG)
  if(NOT OpenSSL_FOUND)
    find_package(OpenSSL REQUIRED)
  endif()

  add_library (
    az_mqtt5_mosquitto
      STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_mosquitto5.c
      ${CMAKE_CURRENT_LIST_DIR}/az_mqtt5_policy.c
  )

  add_compile_definitions(az_mqtt5_mosquitto
    LIBMOSQUITTO_STATIC
  )

  target_link_libraries(az_mqtt5_mosquitto PRIVATE az_core)

  target_link_libraries(az_mqtt5_mosquitto 
    PUBLIC 
      ${MOSQUITTO_LIBRARIES}
      ${PTHREAD_LIBRARIES}
      OpenSSL::SSL 
      OpenSSL::Crypto
  )

  target_include_directories(az_mqtt5_mosquitto INTERFACE ${MOSQUITTO_INCLUDE_DIR})

  # make sure that users can consume the project as a library.
  add_library (az::mqtt5_impl ALIAS az_mqtt5_mosquitto)
endif() # TRANSPORT_MOSQUITTO

# Paho Async Platform

if (TRANSPORT_PAHO AND AZ_PLATFORM_IMPL STREQUAL "POSIX" AND (UNIX AND NOT APPLE))
  message(STATUS "Using Paho Async MQTT transport")
  find_package(eclipse-paho-mqtt-c CONFIG)
  if(NOT eclipse-paho-mqtt-c_FOUND)
    find_package(eclipse-paho-mqtt-c REQUIRED)
  endif()

  find_package(OpenSSL CONFIG)
  if(NOT OpenSSL_FOUND)
    find_package(OpenSSL REQUIRED)
  endif()

  add_library (
    az_mqtt5_paho
      STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_pahoasync.c
      ${CMAKE_CURRENT_LIST_DIR}/az_mqtt5_policy.c
  )

  target_link_libraries(az_mqtt5_paho PRIVATE az_core)

  target_link_libraries(az_mqtt5_paho
    PUBLIC
      ${PTHREAD_LIBRARIES}
      eclipse-paho-mqtt-c::paho-mqtt3as-static
      OpenSSL::SSL 
      OpenSSL::Crypto
  )

  # make sure that users can consume the project as a library.
  add_library (az::mqtt5_impl ALIAS az_mqtt5_paho)
endif() # TRANSPORT_MOSQUITTO

# No platform

if (AZ_MQTT_TRANSPORT_IMPL STREQUAL "NONE" AND (UNIX AND NOT APPLE))
  add_library(
    az_mqtt5_notransport 
      STATIC
      ${CMAKE_CURRENT_LIST_DIR}/az_nomqtt5.c
      ${CMAKE_CURRENT_LIST_DIR}/az_mqtt5_policy.c
  )
  
  target_link_libraries(az_mqtt5_notransport PRIVATE az_core)

  add_library (az::mqtt5_impl ALIAS az_mqtt5_notransport)
endif() # AZ_MQTT_TRANSPORT_IMPL STREQUAL "NONE"
