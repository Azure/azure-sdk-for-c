// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include "az_test_definitions.h"
#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_mqtt5_rpc_server_codec.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_span_internal.h>
#include <azure/iot/az_iot_common.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_COMMAND_NAME "test_command_name"
#define TEST_SERVICE_GROUP_ID "test_service_group_id"
#define TEST_MODEL_ID "test_model_id"
#define TEST_CLIENT_ID "test_server_id"
#define TEST_SUBSCRIPTION_TOPIC_FORMAT_DEFAULT \
  "services/{serviceId}/{executorId}/command/{name}/request"
#define TEST_DEFAULT_SUBSCRIPTION_TOPIC \
  "services/" TEST_MODEL_ID "/" TEST_CLIENT_ID "/command/+/request\0"
#define TEST_DEFAULT_RESPONSE_TOPIC \
  "services/" TEST_MODEL_ID "/" TEST_CLIENT_ID "/command/" TEST_COMMAND_NAME "/request"
#define TEST_DEFAULT_RESPONSE_TOPIC_FAILURE \
  "services/" TEST_MODEL_ID "/" TEST_CLIENT_ID "/command/" TEST_COMMAND_NAME "request"
#define TEST_DEFAULT_FUNGIBLE_SUBSCRIPTION_TOPIC \
  "$share/" TEST_SERVICE_GROUP_ID "/services/" TEST_MODEL_ID "/_any_/command/+/request\0"
#define TEST_CUSTOM_SUBSCRIPTION_TOPIC_FORMAT_1 \
  "controller/{executorId}/service/{serviceId}/command/{name}"
#define TEST_CUSTOM_SUBSCRIPTION_TOPIC_1 \
  "controller/" TEST_CLIENT_ID "/service/" TEST_MODEL_ID "/command/+\0"
#define TEST_CUSTOM_SUBSCRIPTION_TOPIC_FORMAT_2 "controller/{executorId}/service/command/{name}"
#define TEST_CUSTOM_SUBSCRIPTION_TOPIC_2 "controller/" TEST_CLIENT_ID "/service/command/+\0"
#define TEST_CUSTOM_SUBSCRIPTION_TOPIC_FORMAT_3 "controller/service/command/+"
#define TEST_CUSTOM_SUBSCRIPTION_TOPIC_3 "controller/service/command/+\0"

static az_mqtt5_rpc_server_codec test_rpc_server_codec; // TODO_L: Move this to each function

static void test_az_mqtt5_rpc_server_codec_options_default_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_server_codec_options options = az_mqtt5_rpc_server_codec_options_default();

  assert_int_equal(options.subscribe_timeout_in_seconds, AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS);
}

static void test_az_mqtt5_rpc_server_codec_init_no_options_success(void** state)
{
  (void)state;

  assert_int_equal(
      az_mqtt5_rpc_server_codec_init(
          &test_rpc_server_codec,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          NULL),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      test_rpc_server_codec._internal.client_id, AZ_SPAN_FROM_STR(TEST_CLIENT_ID)));
  assert_true(az_span_is_content_equal(
      test_rpc_server_codec._internal.model_id, AZ_SPAN_FROM_STR(TEST_MODEL_ID)));
  assert_true(az_span_is_content_equal(
      test_rpc_server_codec._internal.options.subscription_topic_format,
      AZ_SPAN_FROM_STR(TEST_SUBSCRIPTION_TOPIC_FORMAT_DEFAULT)));
}

static void test_az_mqtt5_rpc_server_codec_init_options_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_server_codec_options options = az_mqtt5_rpc_server_codec_options_default();
  options.subscribe_timeout_in_seconds = 5;
  options.subscription_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIPTION_TOPIC_FORMAT_1);

  assert_int_equal(
      az_mqtt5_rpc_server_codec_init(
          &test_rpc_server_codec,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          &options),
      AZ_OK);

  assert_int_equal(test_rpc_server_codec._internal.options.subscribe_timeout_in_seconds, 5);

  assert_true(az_span_is_content_equal(
      test_rpc_server_codec._internal.options.subscription_topic_format,
      AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIPTION_TOPIC_FORMAT_1)));
}

static void az_mqtt5_rpc_server_codec_get_subscribe_topic_specific_endpoint_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_server_codec_options test_server_options
      = az_mqtt5_rpc_server_codec_options_default();
  assert_int_equal(
      az_mqtt5_rpc_server_codec_init(
          &test_rpc_server_codec,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          &test_server_options),
      AZ_OK);

  char test_subscription_topic_buffer[56]; // Exact size.
  int32_t test_subscription_topic_out_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_server_codec_get_subscribe_topic(
          &test_rpc_server_codec,
          AZ_SPAN_EMPTY,
          test_subscription_topic_buffer,
          sizeof(test_subscription_topic_buffer),
          (size_t*)&test_subscription_topic_out_size),
      AZ_OK);

  az_span test_sub_topic
      = az_span_create((uint8_t*)test_subscription_topic_buffer, test_subscription_topic_out_size);
  az_span test_default_sub_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_SUBSCRIPTION_TOPIC);

  assert_true(az_span_is_content_equal(test_sub_topic, test_default_sub_topic));
}

static void az_mqtt5_rpc_server_codec_get_subscribe_topic_custom_endpoint_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_server_codec_options test_server_options
      = az_mqtt5_rpc_server_codec_options_default();
  test_server_options.subscription_topic_format
      = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIPTION_TOPIC_FORMAT_1);
  assert_int_equal(
      az_mqtt5_rpc_server_codec_init(
          &test_rpc_server_codec,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          &test_server_options),
      AZ_OK);

  char test_subscription_topic_buffer[256];
  int32_t test_subscription_topic_out_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_server_codec_get_subscribe_topic(
          &test_rpc_server_codec,
          AZ_SPAN_EMPTY,
          test_subscription_topic_buffer,
          sizeof(test_subscription_topic_buffer),
          (size_t*)&test_subscription_topic_out_size),
      AZ_OK);

  az_span test_sub_topic
      = az_span_create((uint8_t*)test_subscription_topic_buffer, test_subscription_topic_out_size);
  az_span test_default_sub_topic = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIPTION_TOPIC_1);

  assert_true(az_span_is_content_equal(test_sub_topic, test_default_sub_topic));
}

static void az_mqtt5_rpc_server_codec_get_subscribe_topic_custom_endpoint_executor_only_success(
    void** state)
{
  (void)state;

  az_mqtt5_rpc_server_codec_options test_server_options
      = az_mqtt5_rpc_server_codec_options_default();
  test_server_options.subscription_topic_format
      = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIPTION_TOPIC_FORMAT_2);
  assert_int_equal(
      az_mqtt5_rpc_server_codec_init(
          &test_rpc_server_codec,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          &test_server_options),
      AZ_OK);

  char test_subscription_topic_buffer[256];
  int32_t test_subscription_topic_out_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_server_codec_get_subscribe_topic(
          &test_rpc_server_codec,
          AZ_SPAN_EMPTY,
          test_subscription_topic_buffer,
          sizeof(test_subscription_topic_buffer),
          (size_t*)&test_subscription_topic_out_size),
      AZ_OK);

  az_span test_sub_topic
      = az_span_create((uint8_t*)test_subscription_topic_buffer, test_subscription_topic_out_size);
  az_span test_default_sub_topic = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIPTION_TOPIC_2);

  assert_true(az_span_is_content_equal(test_sub_topic, test_default_sub_topic));
}

static void az_mqtt5_rpc_server_codec_get_subscribe_topic_custom_endpoint_no_elements_success(
    void** state)
{
  (void)state;

  az_mqtt5_rpc_server_codec_options test_server_options
      = az_mqtt5_rpc_server_codec_options_default();
  test_server_options.subscription_topic_format
      = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIPTION_TOPIC_FORMAT_3);
  assert_int_equal(
      az_mqtt5_rpc_server_codec_init(
          &test_rpc_server_codec,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          &test_server_options),
      AZ_OK);

  char test_subscription_topic_buffer[256];
  int32_t test_subscription_topic_out_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_server_codec_get_subscribe_topic(
          &test_rpc_server_codec,
          AZ_SPAN_EMPTY,
          test_subscription_topic_buffer,
          sizeof(test_subscription_topic_buffer),
          (size_t*)&test_subscription_topic_out_size),
      AZ_OK);

  az_span test_sub_topic
      = az_span_create((uint8_t*)test_subscription_topic_buffer, test_subscription_topic_out_size);
  az_span test_default_sub_topic = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIPTION_TOPIC_3);

  assert_true(az_span_is_content_equal(test_sub_topic, test_default_sub_topic));
}

static void az_mqtt5_rpc_server_codec_get_subscribe_topic_custom_endpoint_buffer_size_failure(
    void** state)
{
  (void)state;

  az_mqtt5_rpc_server_codec_options test_server_options
      = az_mqtt5_rpc_server_codec_options_default();
  assert_int_equal(
      az_mqtt5_rpc_server_codec_init(
          &test_rpc_server_codec,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          &test_server_options),
      AZ_OK);

  char test_subscription_topic_buffer[55];
  int32_t test_subscription_topic_out_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_server_codec_get_subscribe_topic(
          &test_rpc_server_codec,
          AZ_SPAN_EMPTY,
          test_subscription_topic_buffer,
          sizeof(test_subscription_topic_buffer),
          (size_t*)&test_subscription_topic_out_size),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void az_mqtt5_rpc_server_codec_get_subscribe_topic_fungible_endpoint_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_server_codec_options test_server_options
      = az_mqtt5_rpc_server_codec_options_default();
  assert_int_equal(
      az_mqtt5_rpc_server_codec_init(
          &test_rpc_server_codec,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          &test_server_options),
      AZ_OK);

  char test_subscription_topic_buffer[76]; // Exact size.
  int32_t test_subscription_topic_out_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_server_codec_get_subscribe_topic(
          &test_rpc_server_codec,
          AZ_SPAN_FROM_STR(TEST_SERVICE_GROUP_ID),
          test_subscription_topic_buffer,
          sizeof(test_subscription_topic_buffer),
          (size_t*)&test_subscription_topic_out_size),
      AZ_OK);

  az_span test_sub_topic
      = az_span_create((uint8_t*)test_subscription_topic_buffer, test_subscription_topic_out_size);
  az_span test_default_sub_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_FUNGIBLE_SUBSCRIPTION_TOPIC);

  assert_true(az_span_is_content_equal(test_sub_topic, test_default_sub_topic));
}

static void az_mqtt5_rpc_server_codec_parse_received_topic_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_server_codec_options test_server_options
      = az_mqtt5_rpc_server_codec_options_default();

  assert_int_equal(
      az_mqtt5_rpc_server_codec_init(
          &test_rpc_server_codec,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          &test_server_options),
      AZ_OK);

  az_span test_response_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_RESPONSE_TOPIC);
  az_mqtt5_rpc_server_codec_request test_request;

  assert_int_equal(
      az_mqtt5_rpc_server_codec_parse_received_topic(
          &test_rpc_server_codec, test_response_topic, &test_request),
      AZ_OK);

  assert_true(az_span_is_content_equal(test_request.service_id, AZ_SPAN_FROM_STR(TEST_MODEL_ID)));
  assert_true(az_span_is_content_equal(test_request.executor_id, AZ_SPAN_FROM_STR(TEST_CLIENT_ID)));
  assert_true(
      az_span_is_content_equal(test_request.command_name, AZ_SPAN_FROM_STR(TEST_COMMAND_NAME)));
}

static void az_mqtt5_rpc_server_codec_parse_received_topic_failure(void** state)
{
  (void)state;

  az_mqtt5_rpc_server_codec_options test_server_options
      = az_mqtt5_rpc_server_codec_options_default();

  assert_int_equal(
      az_mqtt5_rpc_server_codec_init(
          &test_rpc_server_codec,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          &test_server_options),
      AZ_OK);

  az_span test_response_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_RESPONSE_TOPIC_FAILURE);
  az_mqtt5_rpc_server_codec_request test_request;

  assert_int_equal(
      az_mqtt5_rpc_server_codec_parse_received_topic(
          &test_rpc_server_codec, test_response_topic, &test_request),
      AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

int test_az_mqtt5_rpc_server_codec()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_rpc_server_codec_options_default_success),
    cmocka_unit_test(test_az_mqtt5_rpc_server_codec_init_no_options_success),
    cmocka_unit_test(test_az_mqtt5_rpc_server_codec_init_options_success),
    cmocka_unit_test(az_mqtt5_rpc_server_codec_get_subscribe_topic_specific_endpoint_success),
    cmocka_unit_test(az_mqtt5_rpc_server_codec_get_subscribe_topic_custom_endpoint_success),
    cmocka_unit_test(
        az_mqtt5_rpc_server_codec_get_subscribe_topic_custom_endpoint_executor_only_success),
    cmocka_unit_test(
        az_mqtt5_rpc_server_codec_get_subscribe_topic_custom_endpoint_no_elements_success),
    cmocka_unit_test(
        az_mqtt5_rpc_server_codec_get_subscribe_topic_custom_endpoint_buffer_size_failure),
    cmocka_unit_test(az_mqtt5_rpc_server_codec_get_subscribe_topic_fungible_endpoint_success),
    cmocka_unit_test(az_mqtt5_rpc_server_codec_parse_received_topic_success),
    cmocka_unit_test(az_mqtt5_rpc_server_codec_parse_received_topic_failure),
  };
  return cmocka_run_group_tests_name("az_core_mqtt5_rpc_server_codec", tests, NULL, NULL);
}
