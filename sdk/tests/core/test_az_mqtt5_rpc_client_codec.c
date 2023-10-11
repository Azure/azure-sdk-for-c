// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include "az_test_definitions.h"
#include <azure/core/az_mqtt5_rpc_client.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_span_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_COMMAND_NAME "test_command_name"
#define TEST_MODEL_ID "test_model_id"
#define TEST_CLIENT_ID "test_client_id"
#define TEST_SERVER_ID "test_server_id"
#define TEST_RESPONSE_TOPIC                                                   \
  "vehicles/" TEST_MODEL_ID "/commands/" TEST_SERVER_ID "/" TEST_COMMAND_NAME \
  "/__for_" TEST_CLIENT_ID "\0"
#define TEST_REQUEST_TOPIC \
  "vehicles/" TEST_MODEL_ID "/commands/" TEST_SERVER_ID "/" TEST_COMMAND_NAME "\0"
#define TEST_SUBSCRIPTION_TOPIC \
  "vehicles/" TEST_MODEL_ID "/commands/+/" TEST_COMMAND_NAME "/__for_" TEST_CLIENT_ID "\0"
#define TEST_SUBSCRIPTION_TOPIC_FORMAT \
  "vehicles/{serviceId}/commands/{executorId}/{name}/__for_{invokerId}"
#define TEST_REQUEST_TOPIC_FORMAT "vehicles/{serviceId}/commands/{executorId}/{name}"
#define TEST_CUSTOM_REQUEST_TOPIC "controller/" TEST_SERVER_ID "/command/" TEST_COMMAND_NAME "\0"
#define TEST_CUSTOM_SUBSCRIPTION_TOPIC "controller/+/command/" TEST_COMMAND_NAME "\0"
#define TEST_CUSTOM_SUBSCRIPTION_TOPIC_FORMAT "controller/{executorId}/command/{name}"
#define TEST_CUSTOM_REQUEST_TOPIC_FORMAT "controller/{executorId}/command/{name}"

static az_mqtt5_rpc_client_codec test_rpc_client_codec;

static void test_az_mqtt5_rpc_client_codec_options_default_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_client_codec_options options = az_mqtt5_rpc_client_codec_options_default();

  assert_int_equal(options.subscribe_timeout_in_seconds, AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS);
  assert_int_equal(options.publish_timeout_in_seconds, AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS);
}

static void test_az_mqtt5_rpc_client_codec_init_no_options_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];
  char response_topic_buffer[256];
  char request_topic_buffer[256];

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          NULL),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.client_id, AZ_SPAN_FROM_STR(TEST_CLIENT_ID)));
  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.model_id, AZ_SPAN_FROM_STR(TEST_MODEL_ID)));
  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.command_name, AZ_SPAN_FROM_STR(TEST_COMMAND_NAME)));
}

static void test_az_mqtt5_rpc_client_codec_init_options_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];
  char response_topic_buffer[256];
  char request_topic_buffer[256];
  az_mqtt5_rpc_client_codec_options options = az_mqtt5_rpc_client_codec_options_default();
  options.subscribe_timeout_in_seconds = 5;
  options.publish_timeout_in_seconds = 3;
  options.subscription_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIPTION_TOPIC_FORMAT);
  options.request_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_REQUEST_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          &options),
      AZ_OK);

  assert_int_equal(test_rpc_client_codec._internal.options.subscribe_timeout_in_seconds, 5);
  assert_int_equal(test_rpc_client_codec._internal.options.publish_timeout_in_seconds, 3);
  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.subscription_topic,
      AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIPTION_TOPIC)));

  char test_request_topic_buffer[256];
  az_span req_topic = AZ_SPAN_FROM_BUFFER(test_request_topic_buffer);
  az_span_fill(req_topic, ' ');

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_request_topic(
          &test_rpc_client_codec, AZ_SPAN_FROM_STR(TEST_SERVER_ID), AZ_SPAN_EMPTY, req_topic),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      _az_span_trim_whitespace(req_topic), AZ_SPAN_FROM_STR(TEST_CUSTOM_REQUEST_TOPIC)));
}

static void test_az_mqtt5_rpc_client_get_subscription_topic_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];
  char response_topic_buffer[256];
  char request_topic_buffer[256];

  az_mqtt5_rpc_client_codec_options test_client_options
      = az_mqtt5_rpc_client_codec_options_default();
  test_client_options.subscription_topic_format = AZ_SPAN_FROM_STR(TEST_SUBSCRIPTION_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          &test_client_options),
      AZ_OK);

  char test_subscription_topic_buffer[256];
  az_span sub_topic = AZ_SPAN_FROM_BUFFER(test_subscription_topic_buffer);
  int32_t topic_length;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_subscription_topic(
          &test_rpc_client_codec, sub_topic, &topic_length),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      az_span_slice(sub_topic, 0, topic_length), AZ_SPAN_FROM_STR(TEST_SUBSCRIPTION_TOPIC)));
}

static void test_az_mqtt5_rpc_client_get_response_topic_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];
  char response_topic_buffer[256];
  char request_topic_buffer[256];

  az_mqtt5_rpc_client_codec_options test_client_options
      = az_mqtt5_rpc_client_codec_options_default();
  test_client_options.subscription_topic_format = AZ_SPAN_FROM_STR(TEST_SUBSCRIPTION_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          &test_client_options),
      AZ_OK);

  char test_response_topic_buffer[256];
  az_span resp_topic = AZ_SPAN_FROM_BUFFER(test_response_topic_buffer);
  az_span_fill(resp_topic, ' ');
  int32_t resp_topic_len = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_response_topic(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_EMPTY,
          resp_topic,
          &resp_topic_len),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      _az_span_trim_whitespace(resp_topic), AZ_SPAN_FROM_STR(TEST_RESPONSE_TOPIC)));
}

static void test_az_mqtt5_rpc_client_codec_get_request_topic_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];
  char response_topic_buffer[256];
  char request_topic_buffer[256];

  az_mqtt5_rpc_client_codec_options test_client_options
      = az_mqtt5_rpc_client_codec_options_default();
  test_client_options.request_topic_format = AZ_SPAN_FROM_STR(TEST_REQUEST_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          &test_client_options),
      AZ_OK);

  char test_request_topic_buffer[256];
  az_span req_topic = AZ_SPAN_FROM_BUFFER(test_request_topic_buffer);
  az_span_fill(req_topic, ' ');

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_request_topic(
          &test_rpc_client_codec, AZ_SPAN_FROM_STR(TEST_SERVER_ID), AZ_SPAN_EMPTY, req_topic),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      _az_span_trim_whitespace(req_topic), AZ_SPAN_FROM_STR(TEST_REQUEST_TOPIC)));
}

int test_az_mqtt5_rpc_client_codec()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_options_default_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_init_no_options_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_init_options_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_get_subscription_topic_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_get_response_topic_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_get_request_topic_success),

  };
  return cmocka_run_group_tests_name("az_core_mqtt5_rpc_client_codec", tests, NULL, NULL);
}
