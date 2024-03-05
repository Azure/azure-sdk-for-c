// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include "az_test_definitions.h"
#include <azure/core/az_mqtt5_rpc_client.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_span_internal.h>
#include <azure/iot/az_iot_common.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_COMMAND_NAME "test_command_name"
#define TEST_MODEL_ID "test_model_id"
#define TEST_CLIENT_ID "test_client_id"
#define TEST_SERVER_ID "test_server_id"
#define TEST_DEFAULT_PUBLISH_TOPIC_FORMAT \
  "services/{modelId}/{executorId}/command/{commandName}/request"
#define TEST_DEFAULT_PUBLISH_TOPIC \
  "services/" TEST_MODEL_ID "/" TEST_SERVER_ID "/command/" TEST_COMMAND_NAME "/request\0"
#define TEST_CUSTOM_PUBLISH_TOPIC_FORMAT \
  "controller/{modelId}/{executorId}/command/{commandName}/request"
#define TEST_CUSTOM_PUBLISH_TOPIC \
  "controller/" TEST_MODEL_ID "/" TEST_SERVER_ID "/command/" TEST_COMMAND_NAME "/request\0"
#define TEST_DEFAULT_SUBSCRIBE_TOPIC_FORMAT \
  "clients/{invokerClientId}/services/{modelId}/{executorId}/command/{commandName}/response"
#define TEST_DEFAULT_SUBSCRIBE_TOPIC \
  "clients/" TEST_CLIENT_ID "/services/" TEST_MODEL_ID "/+/command/+/response\0"
#define TEST_DEFAULT_RESPONSE_TOPIC                                       \
  "clients/" TEST_CLIENT_ID "/services/" TEST_MODEL_ID "/" TEST_SERVER_ID \
  "/command/" TEST_COMMAND_NAME "/response\0"
#define TEST_DEFAULT_RESPONSE_TOPIC_INBOUND                               \
  "clients/" TEST_CLIENT_ID "/services/" TEST_MODEL_ID "/" TEST_SERVER_ID \
  "/command/" TEST_COMMAND_NAME "/response"
#define TEST_DEFAULT_RESPONSE_TOPIC_FAILURE_INBOUND                       \
  "clients/" TEST_CLIENT_ID "/services/" TEST_MODEL_ID "/" TEST_SERVER_ID \
  "/command/" TEST_COMMAND_NAME "/respons"
#define TEST_CUSTOM_SUBSCRIBE_TOPIC_FORMAT \
  "vehicles/{modelId}/commands/{executorId}/{commandName}/{invokerClientId}"
#define TEST_CUSTOM_SUBSCRIBE_TOPIC "vehicles/" TEST_MODEL_ID "/commands/+/+/" TEST_CLIENT_ID "\0"
#define TEST_CUSTOM_RESPONSE_TOPIC                                                               \
  "vehicles/" TEST_MODEL_ID "/commands/" TEST_SERVER_ID "/" TEST_COMMAND_NAME "/" TEST_CLIENT_ID \
  "\0"

static void test_az_mqtt5_rpc_client_codec_init_no_options_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          NULL),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.client_id, AZ_SPAN_FROM_STR(TEST_CLIENT_ID)));
  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.model_id, AZ_SPAN_FROM_STR(TEST_MODEL_ID)));
  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.options.subscription_topic_format,
      AZ_SPAN_FROM_STR(TEST_DEFAULT_SUBSCRIBE_TOPIC_FORMAT)));
  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.options.request_topic_format,
      AZ_SPAN_FROM_STR(TEST_DEFAULT_PUBLISH_TOPIC_FORMAT)));
}

static void test_az_mqtt5_rpc_client_codec_init_options_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  az_mqtt5_rpc_client_codec_options options = az_mqtt5_rpc_client_codec_options_default();
  options.subscription_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIBE_TOPIC_FORMAT);
  options.request_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_PUBLISH_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          &options),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.client_id, AZ_SPAN_FROM_STR(TEST_CLIENT_ID)));
  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.model_id, AZ_SPAN_FROM_STR(TEST_MODEL_ID)));
  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.options.subscription_topic_format,
      AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIBE_TOPIC_FORMAT)));
  assert_true(az_span_is_content_equal(
      test_rpc_client_codec._internal.options.request_topic_format,
      AZ_SPAN_FROM_STR(TEST_CUSTOM_PUBLISH_TOPIC_FORMAT)));
}

static void test_az_mqtt5_rpc_client_codec_get_publish_topic_success(void** state)
{
  (void)state;
  az_span test_default_pub_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_PUBLISH_TOPIC);
  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          NULL),
      AZ_OK);

  char test_publish_topic_buffer[az_span_size(test_default_pub_topic)];
  size_t test_publish_topic_out_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_publish_topic(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_publish_topic_buffer,
          sizeof(test_publish_topic_buffer),
          &test_publish_topic_out_size),
      AZ_OK);

  az_span test_pub_topic
      = az_span_create((uint8_t*)test_publish_topic_buffer, (int32_t)test_publish_topic_out_size);

  assert_true(az_span_is_content_equal(test_pub_topic, test_default_pub_topic));
}

static void test_az_mqtt5_rpc_client_codec_get_publish_topic_custom_success(void** state)
{
  (void)state;
  az_span test_custom_pub_topic = AZ_SPAN_FROM_STR(TEST_CUSTOM_PUBLISH_TOPIC);
  az_mqtt5_rpc_client_codec test_rpc_client_codec;
  az_mqtt5_rpc_client_codec_options options = az_mqtt5_rpc_client_codec_options_default();
  options.request_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_PUBLISH_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          &options),
      AZ_OK);

  char test_publish_topic_buffer[az_span_size(test_custom_pub_topic)];
  size_t test_publish_topic_out_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_publish_topic(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_publish_topic_buffer,
          sizeof(test_publish_topic_buffer),
          &test_publish_topic_out_size),
      AZ_OK);

  az_span test_pub_topic
      = az_span_create((uint8_t*)test_publish_topic_buffer, (int32_t)test_publish_topic_out_size);

  assert_true(az_span_is_content_equal(test_pub_topic, test_custom_pub_topic));
}

static void test_az_mqtt5_rpc_client_codec_get_publish_topic_buffer_size_failure(void** state)
{
  (void)state;
  az_span test_default_pub_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_PUBLISH_TOPIC);
  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          NULL),
      AZ_OK);

  char test_publish_topic_buffer[az_span_size(test_default_pub_topic) - 1];
  size_t test_publish_topic_out_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_publish_topic(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_publish_topic_buffer,
          sizeof(test_publish_topic_buffer),
          &test_publish_topic_out_size),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void test_az_mqtt5_rpc_client_codec_get_publish_topic_missing_token_failure(void** state)
{
  (void)state;
  az_span test_default_pub_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_PUBLISH_TOPIC);
  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec, AZ_SPAN_FROM_STR(TEST_CLIENT_ID), AZ_SPAN_EMPTY, NULL),
      AZ_OK);

  char test_publish_topic_buffer[az_span_size(test_default_pub_topic)];

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_publish_topic(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_publish_topic_buffer,
          sizeof(test_publish_topic_buffer),
          NULL),
      AZ_ERROR_ARG);
}

static void test_az_mqtt5_rpc_client_codec_get_response_topic_property_success(void** state)
{
  (void)state;
  az_span test_default_response_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_RESPONSE_TOPIC);

  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          NULL),
      AZ_OK);

  char test_response_property_topic_buffer[az_span_size(test_default_response_topic)]; // Exact size
  size_t test_response_property_topic_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_response_property_topic(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_response_property_topic_buffer,
          sizeof(test_response_property_topic_buffer),
          &test_response_property_topic_size),
      AZ_OK);

  az_span test_response_topic = az_span_create(
      (uint8_t*)test_response_property_topic_buffer, (int32_t)test_response_property_topic_size);

  assert_true(az_span_is_content_equal(test_response_topic, test_default_response_topic));
}

static void test_az_mqtt5_rpc_client_codec_get_response_topic_property_custom_success(void** state)
{
  (void)state;
  az_span test_default_response_topic = AZ_SPAN_FROM_STR(TEST_CUSTOM_RESPONSE_TOPIC);
  az_mqtt5_rpc_client_codec test_rpc_client_codec;
  az_mqtt5_rpc_client_codec_options options = az_mqtt5_rpc_client_codec_options_default();
  options.subscription_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIBE_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          &options),
      AZ_OK);

  char test_response_property_topic_buffer[az_span_size(test_default_response_topic)]; // Exact size
  size_t test_response_property_topic_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_response_property_topic(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_response_property_topic_buffer,
          sizeof(test_response_property_topic_buffer),
          &test_response_property_topic_size),
      AZ_OK);

  az_span test_response_topic = az_span_create(
      (uint8_t*)test_response_property_topic_buffer, (int32_t)test_response_property_topic_size);

  assert_true(az_span_is_content_equal(test_response_topic, test_default_response_topic));
}

static void test_az_mqtt5_rpc_client_codec_get_response_topic_property_buffer_size_failure(
    void** state)
{
  (void)state;

  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          NULL),
      AZ_OK);

  char test_response_property_topic_buffer[95];
  size_t test_response_property_topic_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_response_property_topic(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_response_property_topic_buffer,
          sizeof(test_response_property_topic_buffer),
          &test_response_property_topic_size),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void test_az_mqtt5_rpc_client_codec_get_response_topic_property_missing_token_failure(
    void** state)
{
  (void)state;
  az_span test_default_response_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_RESPONSE_TOPIC);
  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec, AZ_SPAN_EMPTY, AZ_SPAN_FROM_STR(TEST_MODEL_ID), NULL),
      AZ_OK);

  char test_response_property_topic_buffer[az_span_size(test_default_response_topic)];
  size_t test_response_property_topic_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_response_property_topic(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_response_property_topic_buffer,
          sizeof(test_response_property_topic_buffer),
          &test_response_property_topic_size),
      AZ_ERROR_ARG);
}

static void test_az_mqtt5_rpc_client_codec_get_subscribe_topic_success(void** state)
{
  (void)state;
  az_span test_default_subscribe_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_SUBSCRIBE_TOPIC);
  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          NULL),
      AZ_OK);

  char test_subscribe_topic_buffer[az_span_size(test_default_subscribe_topic)]; // Exact size
  size_t test_subscribe_topic_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_subscribe_topic(
          &test_rpc_client_codec,
          test_subscribe_topic_buffer,
          sizeof(test_subscribe_topic_buffer),
          &test_subscribe_topic_size),
      AZ_OK);

  az_span test_subscribe_topic
      = az_span_create((uint8_t*)test_subscribe_topic_buffer, (int32_t)test_subscribe_topic_size);

  assert_true(az_span_is_content_equal(test_subscribe_topic, test_default_subscribe_topic));
}

static void test_az_mqtt5_rpc_client_codec_get_subscribe_topic_custom_success(void** state)
{
  (void)state;
  az_span test_default_subscribe_topic = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIBE_TOPIC);

  az_mqtt5_rpc_client_codec test_rpc_client_codec;
  az_mqtt5_rpc_client_codec_options options = az_mqtt5_rpc_client_codec_options_default();
  options.subscription_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIBE_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          &options),
      AZ_OK);

  char test_subscribe_topic_buffer[az_span_size(test_default_subscribe_topic)];
  size_t test_subscribe_topic_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_subscribe_topic(
          &test_rpc_client_codec,
          test_subscribe_topic_buffer,
          sizeof(test_subscribe_topic_buffer),
          &test_subscribe_topic_size),
      AZ_OK);

  az_span test_subscribe_topic
      = az_span_create((uint8_t*)test_subscribe_topic_buffer, (int32_t)test_subscribe_topic_size);

  assert_true(az_span_is_content_equal(test_subscribe_topic, test_default_subscribe_topic));
}

static void test_az_mqtt5_rpc_client_codec_get_subscribe_topic_buffer_size_failure(void** state)
{
  (void)state;
  az_span test_default_subscribe_topic = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIBE_TOPIC);
  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          NULL),
      AZ_OK);

  char
      test_subscribe_topic_buffer[az_span_size(test_default_subscribe_topic) - 1]; // Exact size - 1
  size_t test_subscribe_topic_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_subscribe_topic(
          &test_rpc_client_codec,
          test_subscribe_topic_buffer,
          sizeof(test_subscribe_topic_buffer),
          &test_subscribe_topic_size),
      AZ_ERROR_NOT_ENOUGH_SPACE);
}

static void test_az_mqtt5_rpc_client_codec_get_subscribe_topic_missing_token_failure(void** state)
{
  (void)state;
  az_span test_default_subscribe_topic = AZ_SPAN_FROM_STR(TEST_CUSTOM_SUBSCRIBE_TOPIC);
  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec, AZ_SPAN_EMPTY, AZ_SPAN_FROM_STR(TEST_MODEL_ID), NULL),
      AZ_OK);

  char test_subscribe_topic_buffer[az_span_size(test_default_subscribe_topic)];
  size_t test_subscribe_topic_size = 0;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_get_subscribe_topic(
          &test_rpc_client_codec,
          test_subscribe_topic_buffer,
          sizeof(test_subscribe_topic_buffer),
          &test_subscribe_topic_size),
      AZ_ERROR_ARG);
}

static void test_az_mqtt5_rpc_client_codec_parse_received_topic_success(void** state)
{
  (void)state;

  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          NULL),
      AZ_OK);

  az_span test_received_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_RESPONSE_TOPIC_INBOUND);
  az_mqtt5_rpc_client_codec_request_response test_response;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_parse_received_topic(
          &test_rpc_client_codec, test_received_topic, &test_response),
      AZ_OK);

  assert_true(
      az_span_is_content_equal(test_response.command_name, AZ_SPAN_FROM_STR(TEST_COMMAND_NAME)));
  assert_true(
      az_span_is_content_equal(test_response.executor_id, AZ_SPAN_FROM_STR(TEST_SERVER_ID)));
}

static void test_az_mqtt5_client_codec_parse_received_topic_failure(void** state)
{
  (void)state;

  az_mqtt5_rpc_client_codec test_rpc_client_codec;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_init(
          &test_rpc_client_codec,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          NULL),
      AZ_OK);

  az_span test_received_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_RESPONSE_TOPIC_FAILURE_INBOUND);
  az_mqtt5_rpc_client_codec_request_response test_response;

  assert_int_equal(
      az_mqtt5_rpc_client_codec_parse_received_topic(
          &test_rpc_client_codec, test_received_topic, &test_response),
      AZ_ERROR_IOT_TOPIC_NO_MATCH);
}

int test_az_mqtt5_rpc_client_codec()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_init_no_options_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_init_options_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_get_publish_topic_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_get_publish_topic_custom_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_get_publish_topic_buffer_size_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_get_publish_topic_missing_token_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_get_response_topic_property_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_get_response_topic_property_custom_success),
    cmocka_unit_test(
        test_az_mqtt5_rpc_client_codec_get_response_topic_property_buffer_size_failure),
    cmocka_unit_test(
        test_az_mqtt5_rpc_client_codec_get_response_topic_property_missing_token_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_get_subscribe_topic_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_get_subscribe_topic_custom_success),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_get_subscribe_topic_buffer_size_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_get_subscribe_topic_missing_token_failure),
    cmocka_unit_test(test_az_mqtt5_rpc_client_codec_parse_received_topic_success),
    cmocka_unit_test(test_az_mqtt5_client_codec_parse_received_topic_failure),

  };
  return cmocka_run_group_tests_name("az_core_mqtt5_rpc_client_codec", tests, NULL, NULL);
}
