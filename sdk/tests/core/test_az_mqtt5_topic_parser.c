// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_mqtt5_config.h>
#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_mqtt5_topic_parser_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_SAMPLE_FORMAT                                                               \
  "test/" _az_MQTT5_TOPIC_PARSER_CLIENT_ID_KEY "/" _az_MQTT5_TOPIC_PARSER_SERVICE_ID_KEY \
  "/" _az_MQTT5_RPC_EXECUTOR_ID_KEY "/" _az_MQTT5_TOPIC_PARSER_COMMAND_ID_KEY "/endoftest"
#define TEST_SAMPLE_FORMAT_REPLACED                                                    \
  "$share/test_service_group_id/test/test_client_id/test_service_id/test_executor_id/" \
  "test_command_id/endoftest\0"
static const az_span test_sample_format = AZ_SPAN_LITERAL_FROM_STR(TEST_SAMPLE_FORMAT);
static const az_span test_sample_format_replaced
    = AZ_SPAN_LITERAL_FROM_STR(TEST_SAMPLE_FORMAT_REPLACED);
static const az_span test_service_group_id = AZ_SPAN_LITERAL_FROM_STR("test_service_group_id");
static const az_span test_sample_client_id = AZ_SPAN_LITERAL_FROM_STR("test_client_id");
static const az_span test_sample_service_id = AZ_SPAN_LITERAL_FROM_STR("test_service_id");
static const az_span test_sample_executor_id = AZ_SPAN_LITERAL_FROM_STR("test_executor_id");
static const az_span test_sample_command_id = AZ_SPAN_LITERAL_FROM_STR("test_command_id");

AZ_INLINE az_span test_az_mqtt5_get_key_no_braces(char* key)
{
  az_span key_span = az_span_create_from_str(key);
  return az_span_slice(key_span, 1, az_span_size(key_span) - 1);
}

static void test_az_mqtt5_topic_parser_calculate_hash_success(void** state)
{
  (void)state;

  az_span invoker_client_id_key
      = test_az_mqtt5_get_key_no_braces(_az_MQTT5_TOPIC_PARSER_CLIENT_ID_KEY);
  az_span model_id_key = test_az_mqtt5_get_key_no_braces(_az_MQTT5_TOPIC_PARSER_SERVICE_ID_KEY);
  az_span executor_client_id_key = test_az_mqtt5_get_key_no_braces(_az_MQTT5_RPC_EXECUTOR_ID_KEY);
  az_span command_name_key = test_az_mqtt5_get_key_no_braces(_az_MQTT5_TOPIC_PARSER_COMMAND_ID_KEY);

  assert_int_equal(
      _az_MQTT5_TOPIC_PARSER_CLIENT_ID_HASH,
      _az_mqtt5_topic_parser_calculate_hash(invoker_client_id_key));
  assert_int_equal(
      _az_MQTT5_TOPIC_PARSER_SERVICE_ID_HASH, _az_mqtt5_topic_parser_calculate_hash(model_id_key));
  assert_int_equal(
      _az_MQTT5_TOPIC_PARSER_EXECUTOR_ID_HASH,
      _az_mqtt5_topic_parser_calculate_hash(executor_client_id_key));
  assert_int_equal(
      _az_MQTT5_TOPIC_PARSER_COMMAND_ID_HASH,
      _az_mqtt5_topic_parser_calculate_hash(command_name_key));
}

static void test_az_mqtt5_topic_parser_replace_tokens_in_format_success(void** state)
{
  (void)state;

  uint8_t test_buffer[108];
  az_span test_buffer_span = AZ_SPAN_FROM_BUFFER(test_buffer);
  uint32_t test_size = 0;

  az_result res = _az_mqtt5_topic_parser_replace_tokens_in_format(
      test_buffer_span,
      test_sample_format,
      test_service_group_id,
      test_sample_client_id,
      test_sample_service_id,
      test_sample_executor_id,
      test_sample_command_id,
      &test_size);

  assert_int_equal(res, AZ_OK);

  az_span test_buffer_span_replaced = az_span_create(test_buffer, (int32_t)test_size);

  assert_true(az_span_is_content_equal(test_buffer_span_replaced, test_sample_format_replaced));
}

static void test_az_mqtt5_topic_parser_replace_tokens_in_format_failure(void** state)
{
  (void)state;

  uint8_t test_buffer[1];
  az_span test_buffer_span = AZ_SPAN_FROM_BUFFER(test_buffer);
  uint32_t test_size = 0;

  az_result res = _az_mqtt5_topic_parser_replace_tokens_in_format(
      test_buffer_span,
      test_sample_format,
      test_service_group_id,
      test_sample_client_id,
      test_sample_service_id,
      test_sample_executor_id,
      test_sample_command_id,
      &test_size);

  assert_int_equal(res, AZ_ERROR_NOT_ENOUGH_SPACE);
  assert_int_equal(test_size, 108);
}

static void test_az_mqtt5_topic_parser_replace_tokens_in_format_no_service_group_failure(
    void** state)
{
  (void)state;

  uint8_t test_buffer[1];
  az_span test_buffer_span = AZ_SPAN_FROM_BUFFER(test_buffer);
  uint32_t test_size = 0;

  az_result res = _az_mqtt5_topic_parser_replace_tokens_in_format(
      test_buffer_span,
      test_sample_format,
      AZ_SPAN_EMPTY,
      test_sample_client_id,
      test_sample_service_id,
      test_sample_executor_id,
      test_sample_command_id,
      &test_size);

  assert_int_equal(res, AZ_ERROR_NOT_ENOUGH_SPACE);
  assert_int_equal(test_size, 79);
}

int test_az_mqtt5_topic_parser()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_topic_parser_calculate_hash_success),
    cmocka_unit_test(test_az_mqtt5_topic_parser_replace_tokens_in_format_success),
    cmocka_unit_test(test_az_mqtt5_topic_parser_replace_tokens_in_format_failure),
    cmocka_unit_test(test_az_mqtt5_topic_parser_replace_tokens_in_format_no_service_group_failure),
  };
  return cmocka_run_group_tests_name("az_core_mqtt5_rpc", tests, NULL, NULL);
}