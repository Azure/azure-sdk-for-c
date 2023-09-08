// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include "az_test_definitions.h"
#include <azure/core/az_mqtt5_rpc_server.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_span_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_COMMAND_NAME "test_command_name"
#define TEST_MODEL_ID "test_model_id"
#define TEST_CLIENT_ID "test_server_id"
#define TEST_SUBSCRIPTION_TOPIC \
  "vehicles/" TEST_MODEL_ID "/commands/" TEST_CLIENT_ID "/" TEST_COMMAND_NAME "\0"
#define TEST_CUSTOM_SUBSCRIPTION_TOPIC "controller/"TEST_MODEL_ID"/command/"TEST_COMMAND_NAME"\0"

static az_mqtt5_rpc_server test_rpc_server;

static void test_az_mqtt5_rpc_server_options_default_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_server_options options = az_mqtt5_rpc_server_options_default();

  assert_int_equal(options.subscribe_timeout_in_seconds, AZ_MQTT5_RPC_DEFAULT_TIMEOUT_SECONDS);
}

static void test_az_rpc_server_init_no_options_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];

  assert_int_equal(
      az_rpc_server_init(
          &test_rpc_server,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          NULL),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      test_rpc_server._internal.client_id, AZ_SPAN_FROM_STR(TEST_CLIENT_ID)));
  assert_true(az_span_is_content_equal(
      test_rpc_server._internal.model_id, AZ_SPAN_FROM_STR(TEST_MODEL_ID)));
  assert_true(az_span_is_content_equal(
      test_rpc_server._internal.command_name, AZ_SPAN_FROM_STR(TEST_COMMAND_NAME)));
}

static void test_az_rpc_server_init_options_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];
  az_mqtt5_rpc_server_options options = az_mqtt5_rpc_server_options_default();
  options.subscribe_timeout_in_seconds = 5;
  options.subscription_topic_format = AZ_SPAN_FROM_STR("controller/{executorId}/command/{name}\0");

  assert_int_equal(
      az_rpc_server_init(
          &test_rpc_server,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          &options),
      AZ_OK);

  assert_int_equal(test_rpc_server._internal.options.subscribe_timeout_in_seconds, 5);

  assert_true(az_span_is_content_equal(
      _az_span_trim_whitespace(test_rpc_server._internal.subscription_topic), AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIPTION_TOPIC)));
}

static void test_az_rpc_server_get_subscription_topic_success(void** state)
{
  (void)state;

  char subscription_topic_buffer[256];

  assert_int_equal(
      az_rpc_server_init(
          &test_rpc_server,
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
          NULL),
      AZ_OK);

  char test_subscription_topic_buffer[256];
  az_span sub_topic = AZ_SPAN_FROM_BUFFER(test_subscription_topic_buffer);
  az_span_fill(sub_topic, ' ');

  assert_int_equal(az_rpc_server_get_subscription_topic(&test_rpc_server, sub_topic), AZ_OK);

  assert_true(az_span_is_content_equal(
      _az_span_trim_whitespace(sub_topic), AZ_SPAN_FROM_STR(TEST_SUBSCRIPTION_TOPIC)));
}

int test_az_mqtt5_rpc_server()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_rpc_server_options_default_success),
    cmocka_unit_test(test_az_rpc_server_init_no_options_success),
    cmocka_unit_test(test_az_rpc_server_init_options_success),
    cmocka_unit_test(test_az_rpc_server_get_subscription_topic_success),

  };
  return cmocka_run_group_tests_name("az_core_mqtt5_rpc_server", tests, NULL, NULL);
}
