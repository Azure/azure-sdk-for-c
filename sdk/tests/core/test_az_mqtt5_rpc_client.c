// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include "az_span_private.h"
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
#define TEST_RESPONSE_TOPIC "vehicles/test_model_id/commands/test_server_id/test_command_name/__for_test_client_id\0"
#define TEST_REQUEST_TOPIC "vehicles/test_model_id/commands/test_server_id/test_command_name\0"
#define TEST_SUBSCRIPTION_TOPIC "vehicles/test_model_id/commands/+/test_command_name/__for_test_client_id\0"

static az_mqtt5_rpc_client test_rpc_client;

static void test_az_mqtt5_rpc_client_options_default_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_client_options options = az_mqtt5_rpc_client_options_default();

  assert_int_equal(options.subscribe_timeout_in_seconds, AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS);
}

static void test_az_rpc_client_init_no_options_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];
  char response_topic_buffer[256];
  char request_topic_buffer[256];

  assert_int_equal(
      az_rpc_client_init(
          &test_rpc_client,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          NULL),
      AZ_OK);
}

static void test_az_rpc_client_init_options_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];
  char response_topic_buffer[256];
  char request_topic_buffer[256];
  az_mqtt5_rpc_client_options options = az_mqtt5_rpc_client_options_default();
  options.subscribe_timeout_in_seconds = 5;

  assert_int_equal(
      az_rpc_client_init(
          &test_rpc_client,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          &options),
      AZ_OK);

}

static void test_az_rpc_client_get_subscription_topic_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];
  char response_topic_buffer[256];
  char request_topic_buffer[256];

  assert_int_equal(
      az_rpc_client_init(
          &test_rpc_client,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          NULL),
      AZ_OK);

  char test_subscription_topic_buffer[256];
  az_span sub_topic = AZ_SPAN_FROM_BUFFER(test_subscription_topic_buffer);
  int32_t topic_length;

  assert_int_equal(
      az_rpc_client_get_subscription_topic(
          &test_rpc_client,
          sub_topic,
           &topic_length),
      AZ_OK);

  assert_true(az_span_is_content_equal(az_span_slice(sub_topic, 0, topic_length), AZ_SPAN_FROM_STR(TEST_SUBSCRIPTION_TOPIC)));
}

static void test_az_rpc_client_get_response_topic_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];
  char response_topic_buffer[256];
  char request_topic_buffer[256];

  assert_int_equal(
      az_rpc_client_init(
          &test_rpc_client,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          NULL),
      AZ_OK);

  char test_response_topic_buffer[256];
  az_span resp_topic = AZ_SPAN_FROM_BUFFER(test_response_topic_buffer);

  assert_int_equal(
      az_rpc_client_get_response_topic(
          &test_rpc_client,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          resp_topic),
      AZ_OK);

  assert_true(az_span_is_content_equal(_az_span_trim_whitespace(resp_topic), AZ_SPAN_FROM_STR(TEST_RESPONSE_TOPIC)));
}

static void test_az_rpc_client_get_request_topic_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];
  char response_topic_buffer[256];
  char request_topic_buffer[256];

  assert_int_equal(
      az_rpc_client_init(
          &test_rpc_client,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(response_topic_buffer),
          AZ_SPAN_FROM_BUFFER(request_topic_buffer),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          NULL),
      AZ_OK);

  char test_request_topic_buffer[256];
  az_span req_topic = AZ_SPAN_FROM_BUFFER(test_request_topic_buffer);

  assert_int_equal(
      az_rpc_client_get_request_topic(
          &test_rpc_client,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          req_topic),
      AZ_OK);

  assert_true(az_span_is_content_equal(_az_span_trim_whitespace(req_topic), AZ_SPAN_FROM_STR(TEST_REQUEST_TOPIC)));
}

int test_az_mqtt5_rpc_client()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_rpc_client_options_default_success),
    cmocka_unit_test(test_az_rpc_client_init_no_options_success),
    cmocka_unit_test(test_az_rpc_client_init_options_success),
    cmocka_unit_test(test_az_rpc_client_get_subscription_topic_success),
    cmocka_unit_test(test_az_rpc_client_get_response_topic_success),
    cmocka_unit_test(test_az_rpc_client_get_request_topic_success),

  };
  return cmocka_run_group_tests_name("az_core_mqtt5_rpc_client", tests, NULL, NULL);
}
