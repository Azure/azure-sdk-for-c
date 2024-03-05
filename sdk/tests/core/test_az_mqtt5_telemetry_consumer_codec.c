// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include "az_test_definitions.h"
#include <azure/core/az_mqtt5_telemetry.h>
#include <azure/core/az_mqtt5_telemetry_consumer_codec.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_mqtt5_topic_parser_internal.h>
#include <azure/core/internal/az_span_internal.h>
#include <azure/iot/az_iot_common.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_SERVICE_GROUP_ID "test_service_group_id"
#define TEST_MODEL_ID "test_model_id"
#define TEST_SENDER_ID "test_sender_id"
#define TEST_DEFAULT_CONSUMER_TOPIC "services/" TEST_MODEL_ID "/" TEST_SENDER_ID "/telemetry\0"
#define TEST_DEFAULT_CONSUMER_TOPIC_RECEIVED \
  "services/" TEST_MODEL_ID "/" TEST_SENDER_ID "/telemetry"
#define TEST_DEFAULT_CONSUMER_FUNGIBLE_TOPIC \
  "$share/" TEST_SERVICE_GROUP_ID "/services/" TEST_MODEL_ID "/" TEST_SENDER_ID "/telemetry\0"
#define TEST_CUSTOM_CONSUMER_TOPIC_FORMAT \
  "sender/{senderId}/service/{modelId}/telemetry/{telemetryName}"
#define TEST_CUSTOM_CONSUMER_TOPIC \
  "sender/" TEST_SENDER_ID "/service/" TEST_MODEL_ID "/telemetry/+\0"

static void test_az_mqtt5_telemetry_producer_codec_init_no_options_success(void** state)
{
  (void)state;
  az_mqtt5_telemetry_consumer_codec consumer;
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_init(
          &consumer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_SENDER_ID), NULL),
      AZ_OK);

  assert_true(
      az_span_is_content_equal(consumer._internal.model_id, AZ_SPAN_FROM_STR(TEST_MODEL_ID)));
  assert_true(
      az_span_is_content_equal(consumer._internal.sender_id, AZ_SPAN_FROM_STR(TEST_SENDER_ID)));
  assert_true(az_span_is_content_equal(
      consumer._internal.options.telemetry_topic_format,
      AZ_SPAN_FROM_STR(AZ_MQTT5_TELEMETRY_DEFAULT_TOPIC_FORMAT)));
}

static void test_az_mqtt5_telemetry_consumer_codec_init_options_success(void** state)
{
  (void)state;
  az_mqtt5_telemetry_consumer_codec consumer;
  az_mqtt5_telemetry_consumer_codec_options options
      = az_mqtt5_telemetry_consumer_codec_options_default();
  options.telemetry_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_CONSUMER_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_init(
          &consumer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_SENDER_ID), &options),
      AZ_OK);

  assert_true(
      az_span_is_content_equal(consumer._internal.model_id, AZ_SPAN_FROM_STR(TEST_MODEL_ID)));
  assert_true(
      az_span_is_content_equal(consumer._internal.sender_id, AZ_SPAN_FROM_STR(TEST_SENDER_ID)));
  assert_true(az_span_is_content_equal(
      consumer._internal.options.telemetry_topic_format,
      AZ_SPAN_FROM_STR(TEST_CUSTOM_CONSUMER_TOPIC_FORMAT)));
}

static void test_az_mqtt5_telemetry_consumer_codec_get_subscribe_topic_default_success(void** state)
{
  (void)state;
  az_span test_default_sub_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_CONSUMER_TOPIC);
  az_mqtt5_telemetry_consumer_codec consumer;
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_init(
          &consumer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_SENDER_ID), NULL),
      AZ_OK);

  char test_subscription_topic_buffer[az_span_size(test_default_sub_topic)];
  size_t test_subscription_topic_buffer_out_size = 0;
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_get_subscribe_topic(
          &consumer,
          test_subscription_topic_buffer,
          sizeof(test_subscription_topic_buffer),
          &test_subscription_topic_buffer_out_size),
      AZ_OK);

  az_span test_sub_topic = az_span_create(
      (uint8_t*)test_subscription_topic_buffer, (int32_t)test_subscription_topic_buffer_out_size);

  assert_true(az_span_is_content_equal(test_sub_topic, test_default_sub_topic));
}

static void test_az_mqtt5_telemetry_consumer_codec_get_subscribe_topic_fungible_endpoint_success(
    void** state)
{
  (void)state;
  az_span test_default_sub_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_CONSUMER_FUNGIBLE_TOPIC);
  az_mqtt5_telemetry_consumer_codec consumer;
  az_mqtt5_telemetry_consumer_codec_options options
      = az_mqtt5_telemetry_consumer_codec_options_default();
  options.service_group_id = AZ_SPAN_FROM_STR(TEST_SERVICE_GROUP_ID);
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_init(
          &consumer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_SENDER_ID), &options),
      AZ_OK);

  char test_subscription_topic_buffer[az_span_size(test_default_sub_topic)];
  size_t test_subscription_topic_buffer_out_size = 0;
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_get_subscribe_topic(
          &consumer,
          test_subscription_topic_buffer,
          sizeof(test_subscription_topic_buffer),
          &test_subscription_topic_buffer_out_size),
      AZ_OK);

  az_span test_sub_topic = az_span_create(
      (uint8_t*)test_subscription_topic_buffer, (int32_t)test_subscription_topic_buffer_out_size);

  assert_true(az_span_is_content_equal(test_sub_topic, test_default_sub_topic));
}

static void test_az_mqtt5_telemetry_consumer_codec_get_subscribe_custom_topic_success(void** state)
{
  (void)state;
  az_span test_custom_sub_topic = AZ_SPAN_FROM_STR(TEST_CUSTOM_CONSUMER_TOPIC);
  az_mqtt5_telemetry_consumer_codec consumer;
  az_mqtt5_telemetry_consumer_codec_options options
      = az_mqtt5_telemetry_consumer_codec_options_default();
  options.telemetry_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_CONSUMER_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_init(
          &consumer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_SENDER_ID), &options),
      AZ_OK);

  char test_subscription_topic_buffer[az_span_size(test_custom_sub_topic)];
  size_t test_subscription_topic_buffer_out_size = 0;
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_get_subscribe_topic(
          &consumer,
          test_subscription_topic_buffer,
          sizeof(test_subscription_topic_buffer),
          &test_subscription_topic_buffer_out_size),
      AZ_OK);

  az_span test_sub_topic = az_span_create(
      (uint8_t*)test_subscription_topic_buffer, (int32_t)test_subscription_topic_buffer_out_size);

  assert_true(az_span_is_content_equal(test_sub_topic, test_custom_sub_topic));
}

static void test_az_mqtt5_telemetry_consumer_codec_get_subscribe_topic_buffer_size_failure(
    void** state)
{
  (void)state;
  az_span test_default_sub_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_CONSUMER_TOPIC);
  az_mqtt5_telemetry_consumer_codec consumer;
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_init(
          &consumer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_SENDER_ID), NULL),
      AZ_OK);

  char test_subscription_topic_buffer[az_span_size(test_default_sub_topic) - 1];
  size_t test_subscription_topic_buffer_out_size = 0;
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_get_subscribe_topic(
          &consumer,
          test_subscription_topic_buffer,
          sizeof(test_subscription_topic_buffer),
          &test_subscription_topic_buffer_out_size),
      AZ_ERROR_NOT_ENOUGH_SPACE);

  assert_int_equal(test_subscription_topic_buffer_out_size, az_span_size(test_default_sub_topic));
}

static void test_az_mqtt5_telemetry_consumer_codec_get_subscribe_topic_missing_token_failure(
    void** state)
{
  (void)state;

  az_mqtt5_telemetry_consumer_codec consumer;
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_init(
          &consumer, AZ_SPAN_EMPTY, AZ_SPAN_FROM_STR(TEST_SENDER_ID), NULL),
      AZ_OK);

  char test_subscription_topic_buffer[1];
  size_t test_subscription_topic_buffer_out_size = 0;
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_get_subscribe_topic(
          &consumer,
          test_subscription_topic_buffer,
          sizeof(test_subscription_topic_buffer),
          &test_subscription_topic_buffer_out_size),
      AZ_ERROR_ARG);
}

static void test_az_mqtt5_telemetry_consumer_codec_parse_received_default_topic_success(
    void** state)
{
  (void)state;
  az_mqtt5_telemetry_consumer_codec consumer;
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_init(
          &consumer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_SENDER_ID), NULL),
      AZ_OK);

  az_span test_received_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_CONSUMER_TOPIC_RECEIVED);
  az_mqtt5_telemetry_consumer_codec_data test_data;
  assert_int_equal(
      az_mqtt5_telemetry_consumer_codec_parse_received_topic(
          &consumer, test_received_topic, &test_data),
      AZ_OK);

  assert_true(az_span_is_content_equal(test_data.model_id, AZ_SPAN_FROM_STR(TEST_MODEL_ID)));
  assert_true(az_span_is_content_equal(test_data.sender_id, AZ_SPAN_FROM_STR(TEST_SENDER_ID)));
  assert_true(az_span_is_content_equal(test_data.telemetry_name, AZ_SPAN_EMPTY));
}

int test_az_mqtt5_telemetry_consumer_codec()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_codec_init_no_options_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_consumer_codec_init_options_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_consumer_codec_get_subscribe_topic_default_success),
    cmocka_unit_test(
        test_az_mqtt5_telemetry_consumer_codec_get_subscribe_topic_fungible_endpoint_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_consumer_codec_get_subscribe_custom_topic_success),
    cmocka_unit_test(
        test_az_mqtt5_telemetry_consumer_codec_get_subscribe_topic_buffer_size_failure),
    cmocka_unit_test(
        test_az_mqtt5_telemetry_consumer_codec_get_subscribe_topic_missing_token_failure),
    cmocka_unit_test(test_az_mqtt5_telemetry_consumer_codec_parse_received_default_topic_success),
  };
  return cmocka_run_group_tests_name("az_core_mqtt5_telemetry_consumer_codec", tests, NULL, NULL);
}
