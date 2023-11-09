// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_mqtt5_config.h>
#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_mqtt5_rpc_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_SAMPLE_FORMAT                                           \
  "test/" AZ_MQTT5_RPC_CLIENT_ID_KEY "/" AZ_MQTT5_RPC_SERVICE_ID_KEY \
  "/" AZ_MQTT5_RPC_EXECUTOR_ID_KEY "/" AZ_MQTT5_RPC_COMMAND_ID_KEY "/endoftest"
#define TEST_SAMPLE_FORMAT_REPLACED                                                    \
  "$share/test_service_group_id/test/test_client_id/test_service_id/test_executor_id/" \
  "test_command_id/endoftest\0"
static const az_span test_sample_format = AZ_SPAN_LITERAL_FROM_STR(TEST_SAMPLE_FORMAT);
static const az_span test_service_group_id = AZ_SPAN_LITERAL_FROM_STR("test_service_group_id");
static const az_span test_sample_client_id = AZ_SPAN_LITERAL_FROM_STR("test_client_id");
static const az_span test_sample_service_id = AZ_SPAN_LITERAL_FROM_STR("test_service_id");
static const az_span test_sample_executor_id = AZ_SPAN_LITERAL_FROM_STR("test_executor_id");
static const az_span test_sample_command_id = AZ_SPAN_LITERAL_FROM_STR("test_command_id");

// Calculated hashes shown below. UPDATE whenever tokens change.
static const uint32_t az_mqtt5_rpc_client_id_hash = 3426466449;
static const uint32_t az_mqtt5_rpc_service_id_hash = 4175641829;
static const uint32_t az_mqtt5_rpc_executor_id_hash = 3913329219;
static const uint32_t az_mqtt5_rpc_command_id_hash = 2624200456;

static void test_az_mqtt5_rpc_calculate_hash_success(void** state)
{
  (void)state;

  az_span invoker_client_id_key = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_CLIENT_ID_KEY);
  invoker_client_id_key
      = az_span_slice(invoker_client_id_key, 1, az_span_size(invoker_client_id_key) - 1);
  az_span model_id_key = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_SERVICE_ID_KEY);
  model_id_key = az_span_slice(model_id_key, 1, az_span_size(model_id_key) - 1);
  az_span executor_client_id_key = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_EXECUTOR_ID_KEY);
  executor_client_id_key
      = az_span_slice(executor_client_id_key, 1, az_span_size(executor_client_id_key) - 1);
  az_span command_name_key = AZ_SPAN_LITERAL_FROM_STR(AZ_MQTT5_RPC_COMMAND_ID_KEY);
  command_name_key = az_span_slice(command_name_key, 1, az_span_size(command_name_key) - 1);

  assert_int_equal(
      az_mqtt5_rpc_client_id_hash, _az_mqtt5_rpc_calculate_hash(invoker_client_id_key));
  assert_int_equal(az_mqtt5_rpc_service_id_hash, _az_mqtt5_rpc_calculate_hash(model_id_key));
  assert_int_equal(
      az_mqtt5_rpc_executor_id_hash, _az_mqtt5_rpc_calculate_hash(executor_client_id_key));
  assert_int_equal(az_mqtt5_rpc_command_id_hash, _az_mqtt5_rpc_calculate_hash(command_name_key));
}

static void test_az_mqtt5_rpc_replace_tokens_in_format_success(void** state)
{
  (void)state;

  uint8_t test_buffer[108];
  az_span test_buffer_span = AZ_SPAN_FROM_BUFFER(test_buffer);
  uint32_t test_size = 0;

  az_result res = _az_mqtt5_rpc_replace_tokens_in_format(
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
  az_span test_buffer_compare = AZ_SPAN_FROM_STR(TEST_SAMPLE_FORMAT_REPLACED);

  assert_true(az_span_is_content_equal(test_buffer_span_replaced, test_buffer_compare));
}

static void test_az_mqtt5_rpc_replace_tokens_in_format_failure(void** state)
{
  (void)state;

  uint8_t test_buffer[1];
  az_span test_buffer_span = AZ_SPAN_FROM_BUFFER(test_buffer);
  uint32_t test_size = 0;

  az_result res = _az_mqtt5_rpc_replace_tokens_in_format(
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

static void test_az_mqtt5_rpc_replace_tokens_in_format_no_service_group_failure(void** state)
{
  (void)state;

  uint8_t test_buffer[1];
  az_span test_buffer_span = AZ_SPAN_FROM_BUFFER(test_buffer);
  uint32_t test_size = 0;

  az_result res = _az_mqtt5_rpc_replace_tokens_in_format(
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

int test_az_mqtt5_rpc()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_rpc_calculate_hash_success),
    cmocka_unit_test(test_az_mqtt5_rpc_replace_tokens_in_format_success),
    cmocka_unit_test(test_az_mqtt5_rpc_replace_tokens_in_format_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_replace_tokens_in_format_no_service_group_failure),
  };
  return cmocka_run_group_tests_name("az_core_mqtt5_rpc", tests, NULL, NULL);
}
