# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.10)

project (az_core_test_lib_test LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include(AddCMockaTest)
include(CheckAndIncludeCodeCov)

# Define path to core
set(AZ_CORE_PATH ${CMAKE_SOURCE_DIR}/sdk/src/azure/core)

if (AZ_PLATFORM_IMPL STREQUAL "POSIX" AND NOT APPLE)
  add_library (
    az_core_test_lib
    ${AZ_CORE_PATH}/az_base64.c
    ${AZ_CORE_PATH}/az_base64.c
    ${AZ_CORE_PATH}/az_context.c
    ${AZ_CORE_PATH}/az_hfsm.c
    ${AZ_CORE_PATH}/az_event_pipeline.c
    ${AZ_CORE_PATH}/az_event_policy_collection.c
    ${AZ_CORE_PATH}/az_http_pipeline.c
    ${AZ_CORE_PATH}/az_http_policy.c
    ${AZ_CORE_PATH}/az_http_policy_logging.c
    ${AZ_CORE_PATH}/az_http_policy_retry.c
    ${AZ_CORE_PATH}/az_http_request.c
    ${AZ_CORE_PATH}/az_http_response.c
    ${AZ_CORE_PATH}/az_json_reader.c
    ${AZ_CORE_PATH}/az_json_token.c
    ${AZ_CORE_PATH}/az_json_writer.c
    ${AZ_CORE_PATH}/az_log.c
    ${AZ_CORE_PATH}/az_mqtt5_connection.c
    ${AZ_CORE_PATH}/az_mqtt5_connection_hfsm.c
    ${AZ_CORE_PATH}/az_mqtt5_rpc_server_hfsm.c
    ${AZ_CORE_PATH}/az_mqtt5_rpc_server_codec.c
    ${AZ_CORE_PATH}/az_mqtt5_rpc_client_hfsm.c
    ${AZ_CORE_PATH}/az_mqtt5_rpc_client_codec.c
    ${AZ_CORE_PATH}/az_mqtt5_telemetry_consumer_hfsm.c
    ${AZ_CORE_PATH}/az_mqtt5_telemetry_consumer_codec.c
    ${AZ_CORE_PATH}/az_mqtt5_telemetry_producer_hfsm.c
    ${AZ_CORE_PATH}/az_mqtt5_telemetry_producer_codec.c
    ${AZ_CORE_PATH}/az_mqtt5_topic_parser.c
    ${AZ_CORE_PATH}/az_precondition.c
    ${AZ_CORE_PATH}/az_span.c
  )
else()
  add_library (
    az_core_test_lib
    ${AZ_CORE_PATH}/az_base64.c
    ${AZ_CORE_PATH}/az_context.c
    ${AZ_CORE_PATH}/az_http_pipeline.c
    ${AZ_CORE_PATH}/az_http_policy.c
    ${AZ_CORE_PATH}/az_http_policy_logging.c
    ${AZ_CORE_PATH}/az_http_policy_retry.c
    ${AZ_CORE_PATH}/az_http_request.c
    ${AZ_CORE_PATH}/az_http_response.c
    ${AZ_CORE_PATH}/az_json_reader.c
    ${AZ_CORE_PATH}/az_json_token.c
    ${AZ_CORE_PATH}/az_json_writer.c
    ${AZ_CORE_PATH}/az_log.c
    ${AZ_CORE_PATH}/az_precondition.c
    ${AZ_CORE_PATH}/az_span.c
  )
endif()

target_include_directories(az_core_test_lib 
  PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sdk/inc>
  $<INSTALL_INTERFACE:include/az_core_test_lib_test>
)


# Custom options for the core tests
add_compile_definitions(AZ_MQTT5_USER_CONFIG)

target_include_directories(az_core_test_lib
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/sdk/tests/core/inc>
)

if (UNIX AND NOT APPLE)
  target_link_libraries(az_core_test_lib
  PUBLIC
    ${PAL}
    ${MQTT_TRANSPORT}
  )
else()
  target_link_libraries(az_core_test_lib
    PUBLIC
      ${PAL}
  )
endif()

# TODO: #2479: reason for excluding these files
set(COV_EXCLUDE
  ${az_SOURCE_DIR}/sdk/inc/azure/core/internal/az_http_internal.h
  ${az_SOURCE_DIR}/sdk/inc/azure/core/internal/az_retry_internal.h
  ${az_SOURCE_DIR}/sdk/inc/azure/core/az_http.h
  ${az_SOURCE_DIR}/sdk/src/azure/core/az_precondition.c
  ${az_SOURCE_DIR}/sdk/src/azure/core/az_http_policy.c
  ${az_SOURCE_DIR}/sdk/src/azure/core/az_http_policy_retry.c
  ${az_SOURCE_DIR}/sdk/src/azure/core/az_http_private.h)

create_code_coverage_targets(az_core_test_lib)

# make sure that other testing libraries can consume the project as a library.
add_library (az::core_test_lib ALIAS az_core_test_lib)

# -ld link option is only available for gcc
if(UNIT_TESTING_MOCKS)
    set(WRAP_FUNCTIONS "-Wl,--wrap=az_platform_clock_msec \
                        -Wl,--wrap=az_platform_sleep_msec\
                        -Wl,--wrap=az_platform_timer_create\
                        -Wl,--wrap=az_platform_timer_start\
                        -Wl,--wrap=az_platform_timer_destroy\
                        -Wl,--wrap=az_mqtt5_init\
                        -Wl,--wrap=az_mqtt5_outbound_connect\
                        -Wl,--wrap=az_mqtt5_outbound_sub\
                        -Wl,--wrap=az_mqtt5_outbound_pub\
                        -Wl,--wrap=az_mqtt5_outbound_disconnect\
                        ")
else()
    set(WRAP_FUNCTIONS "")
endif()

set(MATH_LIB_UNIX "")
if (UNIX)
    set(MATH_LIB_UNIX "m")
endif()

if (AZ_PLATFORM_IMPL STREQUAL "POSIX" AND NOT APPLE AND UNIT_TESTING_MOCKS)
    add_cmocka_test(az_core_test_lib_test SOURCES
                    main.c
                    mock_az_mqtt5.c
                    test_az_base64.c
                    test_az_context.c
                    test_az_event_pipeline.c
                    test_az_hfsm.c
                    test_az_http.c
                    test_az_json.c
                    test_az_logging.c
                    test_az_mqtt5_connection.c
                    test_az_mqtt5_topic_parser.c
                    test_az_mqtt5_rpc_client_codec.c
                    test_az_mqtt5_rpc_client_hfsm.c
                    test_az_mqtt5_request_hfsm.c
                    test_az_mqtt5_rpc_server_codec.c
                    test_az_mqtt5_rpc_server_hfsm.c
                    test_az_mqtt5_telemetry_consumer_codec.c
                    test_az_mqtt5_telemetry_consumer_hfsm.c
                    test_az_mqtt5_telemetry_producer_codec.c
                    test_az_mqtt5_telemetry_producer_hfsm.c
                    test_az_pipeline.c
                    test_az_policy.c
                    test_az_span.c
                    test_az_url_encode.c
                    COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS} ${NO_CLOBBERED_WARNING}
                    LINK_LIBRARIES ${CMOCKA_LIB} ${MATH_LIB_UNIX} az_core_test_lib ${PAL} az_nohttp
                    LINK_OPTIONS ${WRAP_FUNCTIONS}  
                    # include cmocka headers and private folder headers
                    INCLUDE_DIRECTORIES ${CMOCKA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/
                    )
else()
    add_cmocka_test(az_core_test_lib_test SOURCES
                    main.c
                    test_az_base64.c
                    test_az_context.c
                    test_az_http.c
                    test_az_json.c
                    test_az_logging.c
                    test_az_pipeline.c
                    test_az_policy.c
                    test_az_span.c
                    test_az_url_encode.c
                    COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS} ${NO_CLOBBERED_WARNING}
                    LINK_LIBRARIES ${CMOCKA_LIB} ${MATH_LIB_UNIX} az_core_test_lib ${PAL} az_nohttp
                    LINK_OPTIONS ${WRAP_FUNCTIONS}  
                    # include cmocka headers and private folder headers
                    INCLUDE_DIRECTORIES ${CMOCKA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/sdk/src/azure/core/
                    )
endif()

target_link_libraries(az_core_test_lib_test
  PUBLIC
    az_iot_common
)

create_map_file(az_core_test_lib_test az_core_test_lib_test.map)

add_cmocka_test_environment(az_core_test_lib_test)
